#ifndef AVE_CORE_ENGINE_H
#define AVE_CORE_ENGINE_H


namespace ave {

class GameRunner {
public:

    // return if succees
    virtual bool init( void* ) = 0;

    // return if continue
    virtual bool loop( void* ) = 0;

    GameRunner() = default;

    GameRunner(const GameRunner&) = default;
    GameRunner(GameRunner&&) = default;
    GameRunner& operator=(const GameRunner&) = default;
    GameRunner& operator=(GameRunner&&) = default;

    virtual ~GameRunner() = default;
};

}

#endif