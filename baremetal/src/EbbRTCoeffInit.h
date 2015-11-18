//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef APPS_PINGPONG_BAREMETAL_SRC_EBBRTCOEFFINIT_H_
#define APPS_PINGPONG_BAREMETAL_SRC_EBBRTCOEFFINIT_H_

#include <string>
#include <string.h>

#include <ebbrt/Message.h>

using namespace ebbrt;

class EbbRTCoeffInit : public ebbrt::Messagable<EbbRTCoeffInit> {
    EbbId ebbid;

 public:
    explicit EbbRTCoeffInit(ebbrt::Messenger::NetworkId nid, EbbId id)
    : Messagable<EbbRTCoeffInit>(id), remote_nid_(std::move(nid)) 
    {
	ebbid = id;
    }

  static EbbRTCoeffInit& HandleFault(ebbrt::EbbId id);

  void Print(const char* string);
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf>&& buffer);

 private:
  ebbrt::Messenger::NetworkId remote_nid_;
};

#endif
