#ifndef __TEXT_MODEL_H
#define __TEXT_MODEL_H

#include <memory>
#include "ComProtocol.h"
#include "TcpTextSession.h"
// #include "TcpServer.h"
#include "TcpTextClient.h"
#include "EventLoop.h"
#include "UdpTextClient.h"
// #include "UdpServer.h"
#include "Server.h"

namespace chw {

class TextModel : public workmodel
{
public:
    using Ptr = std::shared_ptr<TextModel>;
    TextModel(const chw::EventLoop::Ptr& poller = nullptr);
    ~TextModel() override;

    void startmodel() override;

private:
    chw::Server::Ptr _pTcpServer;
    chw::TcpTextClient::Ptr _pTcpClient;

};

}//namespace chw 

#endif//__TEXT_MODEL_H