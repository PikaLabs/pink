#ifndef PINK_PB_THREAD_H_
#define PINK_PB_THREAD_H_

#include <string>


class PbConn;

template <typename T>
class PbThread : public Thread
{
public:
    PbThread();
    virtual ~PbThread();

private:
    virtual void *ThreadMain(); 

    typedef std::function<void(void)> handler;
    int Install(std::string, handler);
    std::map<std::string, handler> pbHandler_; 

}

#endif
