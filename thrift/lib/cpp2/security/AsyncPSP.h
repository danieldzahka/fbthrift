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

#pragma once

#include <fizz/client/AsyncFizzClient.h>
#include <fizz/protocol/AsyncFizzBase.h>
#include <folly/futures/Promise.h>
#include <folly/io/async/DelayedDestruction.h>
#include <thrift/lib/cpp2/security/AsyncStopTLS.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersClientExtension.h>

struct psp_ynl;

namespace apache::thrift {
class AsyncPspUpgrade : public folly::DelayedDestruction,
                        private fizz::AsyncFizzBase::ReadCallback {
 public:
  class Callback {
   public:
    virtual void PspUpgradeSuccess() = 0;
    virtual void PspUpgradeError(const folly::exception_wrapper& ew) = 0;
    virtual ~Callback() = default;
  };

  explicit AsyncPspUpgrade(Callback& awaiter, struct psp_ynl* pynl)
      : awaiter_(&awaiter), pynl_{pynl} {}

  void start(fizz::AsyncFizzBase* transport, std::chrono::milliseconds timeout);

 private:
  bool isBufferMovable() noexcept override { return true; }
  void getReadBuffer(void**, size_t* lenReturn) override;
  void readDataAvailable(size_t) noexcept override;
  void readEOF() noexcept override;
  void readBufferAvailable(std::unique_ptr<folly::IOBuf>) noexcept override;
  void readErr(const folly::AsyncSocketException& ex) noexcept override;

  void PspUpgradeTimeoutExpired() noexcept;

  Callback* prepareForTerminalCallback() noexcept {
    if (transactionTimeout_) {
      transactionTimeout_.reset();
    }

    transport_->setReadCB(nullptr);
    return std::exchange(awaiter_, nullptr);
  }

  Callback* awaiter_{nullptr};
  struct psp_ynl* pynl_{nullptr};
  fizz::AsyncFizzBase* transport_{nullptr};
  std::unique_ptr<folly::AsyncTimeout> transactionTimeout_{nullptr};
};

class PspConnector : public fizz::client::AsyncFizzClient::HandshakeCallback,
                     public AsyncStopTLS::Callback,
                     public AsyncPspUpgrade::Callback {
 public:
  explicit PspConnector(struct psp_ynl* pynl) : pynl_{pynl} {}

  folly::AsyncTransport::UniquePtr connect(
      const folly::SocketAddress& address, folly::EventBase* eb);

  void fizzHandshakeSuccess(
      fizz::client::AsyncFizzClient* client) noexcept override;

  void fizzHandshakeError(
      fizz::client::AsyncFizzClient* /* unused */,
      folly::exception_wrapper ex) noexcept override;

  void stopTLSSuccess(std::unique_ptr<folly::IOBuf> endOfData) override;
  void stopTLSError(const folly::exception_wrapper& ew) override;

  void PspUpgradeSuccess() override;
  void PspUpgradeError(const folly::exception_wrapper& ew) override;

 private:
  fizz::client::AsyncFizzClient::UniquePtr client_;
  folly::Promise<folly::AsyncTransport::UniquePtr> promise_;
  folly::EventBase* eb_;
  apache::thrift::AsyncStopTLS::UniquePtr stoptls_;
  std::unique_ptr<apache::thrift::AsyncPspUpgrade> psp_handshake_;
  struct psp_ynl* pynl_{nullptr};
  std::shared_ptr<ThriftParametersClientExtension> extension_;
};
} // namespace apache::thrift
