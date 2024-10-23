#ifndef __TEXT_MODEL_H
#define __TEXT_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "Server.h"
#include "Client.h"

namespace chw {

/**
 *  文本模式，客户端和服务端可以通过命令行><文本聊天。
 */
class TextModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<TextModel>;
    TextModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~TextModel() override;

    virtual void startmodel() override;
    virtual void prepare_exit() override{};

private:
    chw::Server::Ptr _pServer;
    chw::Client::Ptr _pClient;
};

}//namespace chw 

#endif//__TEXT_MODEL_H