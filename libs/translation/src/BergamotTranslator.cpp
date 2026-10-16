/*
 * Author: Runor Ewhro
 * Description: translates with Bergamot on one background thread, loading
 *              each model the first time a translation needs it.
 */
#include <kstack/translation/Translator.hpp>

#include <translator/parser.h>
#include <translator/response_options.h>
#include <translator/service.h>
#include <translator/translation_model.h>

#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <thread>

namespace kstack::translation {
namespace {

namespace bergamot = marian::bergamot;

class BergamotTranslator final : public Translator {
public:
    explicit BergamotTranslator(std::vector<ModelFiles> models)
        : models_(std::move(models)), loaded_(models_.size()) {
        // Marian aborts the process on a bad model unless told to throw
        marian::setThrowExceptionOnAbort(true);
        worker_ = std::thread([this] { run(); });
    }

    ~BergamotTranslator() override {
        {
            const std::lock_guard lock(mutex_);
            stopping_ = true;
            jobs_.clear();
        }
        wake_.notify_one();
        worker_.join();
    }

    bool canTranslate(std::string_view source, std::string_view target) const override {
        return !findRoute(models_, source, target).empty();
    }

    bool translate(std::string text, std::string source, std::string target, Done done) override {
        auto route = findRoute(models_, source, target);
        if (route.empty()) {
            return false;
        }
        {
            const std::lock_guard lock(mutex_);
            jobs_.push_back(Job{std::move(text), std::move(route), std::move(done)});
        }
        wake_.notify_one();
        return true;
    }

private:
    struct Job {
        std::string text;
        std::vector<std::size_t> route;
        Done done;
    };

    std::shared_ptr<bergamot::TranslationModel> model(std::size_t index) {
        if (!loaded_[index]) {
            const auto& config = models_[index].config;
            auto options = bergamot::parseOptionsFromFilePath(config.string(), false);
            // the published configs leave out Bergamot's batching limits,
            // and reserve far more working memory than a clipboard line needs
            options->set("max-length-break", 128);
            options->set("mini-batch-words", 1024);
            options->set("workspace", 128);
            options->set("ssplit-mode", std::string("paragraph"));
            loaded_[index] = std::make_shared<bergamot::TranslationModel>(options);
        }
        return loaded_[index];
    }

    std::optional<std::string> translateNow(const Job& job) {
        std::vector<std::string> sources{job.text};
        const std::vector<bergamot::ResponseOptions> options(1);
        std::vector<bergamot::Response> responses =
            job.route.size() == 1
                ? service_->translateMultiple(model(job.route[0]), std::move(sources), options)
                : service_->pivotMultiple(model(job.route[0]), model(job.route[1]),
                                          std::move(sources), options);
        if (responses.empty()) {
            return std::nullopt;
        }
        return std::move(responses.front().target.text);
    }

    void run() {
        // the service and models belong to this thread alone
        service_ = std::make_unique<bergamot::BlockingService>(bergamot::BlockingService::Config{});
        for (;;) {
            std::unique_lock lock(mutex_);
            wake_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
            if (stopping_) {
                break;
            }
            Job job = std::move(jobs_.front());
            jobs_.pop_front();
            lock.unlock();

            std::optional<std::string> result;
            try {
                result = translateNow(job);
            } catch (const std::exception&) {
                // a broken or mismatched model translates nothing rather than
                // ending the app
            }
            job.done(std::move(result));
        }
        loaded_.clear();
        service_.reset();
    }

    const std::vector<ModelFiles> models_;
    std::vector<std::shared_ptr<bergamot::TranslationModel>> loaded_;
    std::unique_ptr<bergamot::BlockingService> service_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<Job> jobs_;
    bool stopping_ = false;
    std::thread worker_;
};

} // namespace

std::unique_ptr<Translator> createTranslator(std::vector<ModelFiles> models) {
    return std::make_unique<BergamotTranslator>(std::move(models));
}

} // namespace kstack::translation
