#ifndef _EBBRT_COEFF_INIT_H_
#define _EBBRT_COEFF_INIT_H_

#include <math.h>
#include <stdlib.h>
#include <string>

#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/SharedEbb.h>
#include <ebbrt/Message.h>
#include <ebbrt/UniqueIOBuf.h>
#include <iostream>

#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace ebbrt;

class EbbRTCoeffInit;
typedef EbbRef<EbbRTCoeffInit> EbbRTCoeffInitEbbRef;

class EbbRTCoeffInit : public SharedEbb<EbbRTCoeffInit>,
                       public Messagable<EbbRTCoeffInit> {

  EbbId ebbid;
  std::string bmnid;
  std::vector<ebbrt::Messenger::NetworkId> nids;
  int numNodes;
  int recv_counter;

 
  // this is used to save and load context
  ebbrt::EventManager::EventContext* emec{nullptr};

  // a void promise where we want to start some computation after some other
  // function has finished
  ebbrt::Promise<void> mypromise;
  ebbrt::Promise<void> nodesinit;

 public:
  EbbRTCoeffInit(EbbId _ebbid, int _num)
      : Messagable<EbbRTCoeffInit>(_ebbid) {
    ebbid = _ebbid;
    bmnid = "";
    numNodes = _num;
    recv_counter = 0;
    nids.clear();
  }

  // Create function that returns a future ebbref
  static ebbrt::Future<EbbRTCoeffInitEbbRef>
  Create(int num) {
    // calls the constructor in this class and returns a EbbRef
    auto id = ebb_allocator->Allocate();
    auto ebbref = SharedEbb<EbbRTCoeffInit>::Create(
        new EbbRTCoeffInit(id, num), id);

    // returns a future for the EbbRef in AppMain.cc
    return ebbrt::global_id_map
        ->Set(id, ebbrt::messenger->LocalNetworkId().ToBytes())
        // the Future<void> f is returned by the ebbrt::global_id_map
        .Then([ebbref](ebbrt::Future<void> f) {
          // f.Get() ensures the gobal_id_map has completed
          f.Get();
          return ebbref;
        });
  }

  // this uses the void promise to return a future that will
  // get invoked when mypromose gets set with some value
  ebbrt::Future<void> waitReceive() { return std::move(mypromise.GetFuture()); }
  // FIXME
  ebbrt::Future<void> waitNodes() { return std::move(nodesinit.GetFuture()); }

  void addNid(ebbrt::Messenger::NetworkId nid) {
      nids.push_back(nid);
      if ((int)nids.size() == numNodes) {
	  nodesinit.SetValue();
      }
  }

  void Print(ebbrt::Messenger::NetworkId nid, const char* str);
  
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf>&& buffer);

  EbbId getEbbId() { return ebbid; }

  void runJob(int);

  void destroy() { delete this; }
};

#endif
