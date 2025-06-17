/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <psp_ynl.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/security/AsyncPSP.h>
#include <thrift/lib/cpp2/security/SSLUtil.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersClientExtension.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersContext.h>

namespace apache::thrift {
void AsyncPspUpgrade::start(
    fizz::AsyncFizzBase* transport, std::chrono::milliseconds timeout) {
  DestructorGuard guard(this);

  transport_ = transport;
  transport->setReadCB(this);

  if (timeout.count() > 0) {
    auto evb = transport->getEventBase();
    CHECK(transport->getEventBase());
    transactionTimeout_ = folly::AsyncTimeout::make(
        *evb, [this]() noexcept { this->PspUpgradeTimeoutExpired(); });

    transactionTimeout_->scheduleTimeout(timeout);
  }

  auto sock = transport->getUnderlyingTransport<folly::AsyncSocket>();
  if (!sock) {
    LOG(FATAL) << "could not downcast to sock";
  }

  int fd = sock->getNetworkSocket().toFd();
  struct psp_key_parsed rx_key;
  if (psp_ynl_rx_spi_alloc(pynl_, fd, PSP_V0, &rx_key)) {
    LOG(FATAL) << "could not generate psp key\n";
  }

  transport->writeChain(
      nullptr, folly::IOBuf::copyBuffer(&rx_key, sizeof(rx_key)));
}

void AsyncPspUpgrade::PspUpgradeTimeoutExpired() noexcept {
  static folly::Indestructible<folly::exception_wrapper> exc{
      folly::make_exception_wrapper<std::runtime_error>(
          "AsyncPspUpgrade: timeout expired")};
  prepareForTerminalCallback()->PspUpgradeError(*exc);
}

void AsyncPspUpgrade::getReadBuffer(void**, size_t* lenReturn) {
  LOG(DFATAL)
      << "AsyncPspUpgrade::getReadBuffer is being invoked, when it clearly supports movable buffer API";
  *lenReturn = 0;
}

void AsyncPspUpgrade::readDataAvailable(size_t) noexcept {
  LOG(DFATAL)
      << "AsyncPspUpgrade::readDataAvailable is being invoked, when it clearly supports movable buffer API";
}

void AsyncPspUpgrade::readEOF() noexcept {
  prepareForTerminalCallback()->PspUpgradeError(
      folly::make_exception_wrapper<std::runtime_error>(
          "stoptls protocol error: readEOF() before negotiation completion"));
}

void AsyncPspUpgrade::readBufferAvailable(
    std::unique_ptr<folly::IOBuf> tx_key) noexcept {
  tx_key->coalesce();
  if (tx_key->length() != sizeof(struct psp_key_parsed)) {
    prepareForTerminalCallback()->PspUpgradeError(
        folly::make_exception_wrapper<std::runtime_error>(
            "psp protocol error: unexpected rx bytelength"));
    return;
  }

  struct psp_key_parsed key;
  memcpy(&key, tx_key->data(), sizeof(struct psp_key_parsed));

  auto sock = transport_->getUnderlyingTransport<folly::AsyncSocket>();
  if (!sock)
    LOG(FATAL) << "could not downcast to sock";

  int fd = sock->getNetworkSocket().toFd();

  if (psp_ynl_tx_spi_set(pynl_, fd, PSP_V0, &key))
    LOG(FATAL) << "could not insert psp tx key";

  prepareForTerminalCallback()->PspUpgradeSuccess();
}

void AsyncPspUpgrade::readErr(const folly::AsyncSocketException& ex) noexcept {
  prepareForTerminalCallback()->PspUpgradeError(ex);
}

folly::AsyncTransport::UniquePtr PspConnector::connect(
    const folly::SocketAddress& address, folly::EventBase* eb) {
  eb_ = eb;

  auto sock = folly::AsyncSocket::newSocket(eb_, address);
  auto ctx = std::make_shared<fizz::client::FizzClientContext>();
  ctx->setSupportedAlpns({"rs"});
  auto thriftParametersContext =
      std::make_shared<apache::thrift::ThriftParametersContext>();

  bool const have_psp = pynl_ != nullptr;
  thriftParametersContext->setUsePSP(have_psp);
  extension_ =
      std::make_shared<apache::thrift::ThriftParametersClientExtension>(
          thriftParametersContext);

  client_.reset(new fizz::client::AsyncFizzClient(
      std::move(sock), std::move(ctx), extension_));
  client_->connect(
      this,
      nullptr,
      folly::none,
      folly::none,
      folly::Optional<std::vector<fizz::ech::ParsedECHConfig>>(folly::none),
      std::chrono::milliseconds(100));
  return promise_.getFuture().getVia(eb_);
}

void PspConnector::fizzHandshakeSuccess(
    fizz::client::AsyncFizzClient* client) noexcept {
  if (extension_ && extension_->getNegotiatedUsePSP()) {
    LOG(INFO) << "Both client and server support and will use PSP";
    psp_handshake_.reset(new apache::thrift::AsyncPspUpgrade(*this, pynl_));
    psp_handshake_->start(client_.get(), std::chrono::milliseconds(0));
  } else {
    LOG(INFO) << "PSP not negotiated. Sticking with TLS";
    promise_.setValue(std::move(client_));
  }
}

void PspConnector::fizzHandshakeError(
    fizz::client::AsyncFizzClient* /* unused */,
    folly::exception_wrapper ex) noexcept {
  promise_.setException(ex);
  LOG(FATAL) << "failure handshake";
}

void PspConnector::stopTLSSuccess(std::unique_ptr<folly::IOBuf> endOfData) {
  auto plaintext = apache::thrift::moveToPlaintext(client_.get());
  promise_.setValue(std::move(plaintext));
}
void PspConnector::stopTLSError(const folly::exception_wrapper& ew) {
  LOG(ERROR) << "StopTLS accept error. ex=" << folly::exceptionStr(ew);
}

void PspConnector::PspUpgradeSuccess() {
  LOG(INFO) << "Finished PSP upgrade";
  stoptls_.reset(new apache::thrift::AsyncStopTLS(*this));
  stoptls_->start(
      client_.get(),
      apache::thrift::AsyncStopTLS::Role::Client,
      std::chrono::milliseconds(0));
}

void PspConnector::PspUpgradeError(const folly::exception_wrapper& ew) {
  LOG(ERROR) << "psp upgrade error. ex=" << folly::exceptionStr(ew);
}

} // namespace apache::thrift
