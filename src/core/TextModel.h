#ifndef __TEXT_MODEL_H
#define __TEXT_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "EventLoop.h"
#include "Server.h"
#include "Client.h"

namespace chw {

class TextModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<TextModel>;
    TextModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~TextModel() override;

    void startmodel() override;

private:
    chw::Server::Ptr _pServer;
    chw::Client::Ptr _pClient;
};

}//namespace chw 

#endif//__TEXT_MODEL_H