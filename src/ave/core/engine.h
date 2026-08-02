#ifndef AVE_CORE_ENGINE_H
#define AVE_CORE_ENGINE_H

#include <memory>


namespace ave {


// Runner defines something that loops for a long time and then stop.
class Runner {
public:
    /**
     * @param       The int parameter is used to pass information between Runner.
     *              Each Runner class should defined it's own parameter usage. If you don't need
     *              it, set to 0.
     * @return      true if should continue
     */
    virtual bool loop( int ) {
        return false;
    }

    Runner() = default;
    virtual ~Runner() = default;

    Runner(const Runner&) = default;
    Runner(Runner&&) = default;
    Runner& operator=(const Runner&) = default;
    Runner& operator=(Runner&&) = default;
};


class RunnerContainer : public Runner {
public:
    RunnerContainer() = default;
    virtual ~RunnerContainer() = default;

    RunnerContainer(const RunnerContainer&) = delete;
    RunnerContainer& operator=(const RunnerContainer&) = delete;

    RunnerContainer(RunnerContainer&&) = default;
    RunnerContainer& operator=(RunnerContainer&&) = default;

    void set_runner(std::unique_ptr<Runner> runner) {
        runner_ = std::move(runner);
    }

    Runner* get_runner() const noexcept {
        return runner_.get();
    }

private:
    std::unique_ptr<Runner> runner_;
};


}

#endif