//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "EbbRTCoeffInit.h"
#include <vector>
#include <iostream>
#include <fstream>

#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/SpinBarrier.h>
#include <ebbrt/MulticoreEbb.h>
#include <ebbrt/EbbRef.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

EBBRT_PUBLISH_TYPE(, EbbRTCoeffInit);

using namespace ebbrt;

EbbRTCoeffInit& EbbRTCoeffInit::HandleFault(ebbrt::EbbId id) {
  {
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      auto& pr = *boost::any_cast<EbbRTCoeffInit*>(accessor->second);
      ebbrt::EbbRef<EbbRTCoeffInit>::CacheRef(id, pr);
      return pr;
    }
  }

  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  EbbRTCoeffInit* p;
  f.Then([&f, &context, &p, id](ebbrt::Future<std::string> inner) {
    p = new EbbRTCoeffInit(ebbrt::Messenger::NetworkId(inner.Get()), id);
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<EbbRTCoeffInit>::CacheRef(id, *p);
    return *p;
  }

  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& pr = *boost::any_cast<EbbRTCoeffInit*>(accessor->second);
  ebbrt::EbbRef<EbbRTCoeffInit>::CacheRef(id, pr);
  return pr;
}

void EbbRTCoeffInit::Print(const char* str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);

  ebbrt::kprintf("Sending %d bytes\n", buf->ComputeChainDataLength());
  
  SendMessage(remote_nid_, std::move(buf));
}

struct membuf : std::streambuf {
  membuf(char* begin, char* end) { this->setg(begin, begin, end); }
};

static size_t indexToCPU(size_t i) { return i; }

void EbbRTCoeffInit::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                                    std::unique_ptr<ebbrt::IOBuf>&& buffer) 
{
  auto output = std::string(reinterpret_cast<const char*>(buffer->Data()),
                            buffer->Length());

  if (output[0] == 'G') {
    int start, end;

    // deserialize
    ebbrt::IOBuf::DataPointer dp = buffer->GetDataPointer();
    char* t = (char*)(dp.Get(buffer->ComputeChainDataLength()));
    membuf sb{t + 2, t + buffer->ComputeChainDataLength()};
    std::istream stream{&sb};
    boost::archive::text_iarchive ia(stream);

    ia& start& end;

    std::vector<int> test(end - start);

    for (int k = 0; k < (end - start); k++) {
      ia& test[k];
    }

    ebbrt::kprintf("Received msg length: %d bytes\n", buffer->Length());
    ebbrt::kprintf("Number chain elements: %d\n", buffer->CountChainElements());
    ebbrt::kprintf("Computed chain length: %d bytes\n",
                   buffer->ComputeChainDataLength());

    // get number of cores/cpus on backend
    size_t ncpus = ebbrt::Cpu::Count();
    // get my cpu id
    size_t theCpu = ebbrt::Cpu::GetMine();

    // create a spin barrier on all cpus
    static ebbrt::SpinBarrier bar(ncpus);

    ebbrt::kprintf("number cpus: %d, mycpu: %d\n", ncpus, theCpu);

    // gets current context
    ebbrt::EventManager::EventContext context;

    // atomic type with value 0
    std::atomic<size_t> count(0);

    for (size_t i = 0; i < ncpus; i++) {
      // spawn jobs on each core using SpawnRemote
      ebbrt::event_manager->SpawnRemote(
          [theCpu, ncpus, &count, &test, i, &context, start, end]() {
            // get my cpu id
            size_t mycpu = ebbrt::Cpu::GetMine();

            int starte, ende, factor;
            factor = (int)ceil(test.size() / (float)ncpus);
            starte = i * factor;
            ende = i * factor + factor;
            ende = ((size_t)ende > test.size()) ? test.size() : ende;

            ebbrt::kprintf("theCpu: %d, mycpu: %d, start: %d, end: %d, "
                           "test.size(): %d, starte: %d, ende: %d\n",
                           theCpu, mycpu, start, end, test.size(), starte,
                           ende);

            for (int j = starte; j < ende; j++) {
              test[j] *= 2;
            }

            // atomically increment count
            count++;

            // barrier here ensures all cores run until this point
            bar.Wait();

            // basically wait until all cores reach this point
            while (count < (size_t)ncpus)
              ;

            // the cpu that initiated the SpawnRemote has the SaveContext
            if (mycpu == theCpu) {
              // activate context will return computation to instruction
              // after SaveContext below
              ebbrt::event_manager->ActivateContext(std::move(context));
            }
          },
          indexToCPU(
              i));  // if i don't add indexToCPU, one of the cores never run??
    }

    ebbrt::event_manager->SaveContext(context);

    ebbrt::kprintf("Context restored...\n");

    // serialize
    std::ostringstream ofs;
    boost::archive::text_oarchive oa(ofs);
    oa& test& start& end;
    std::string ts = "G " + ofs.str();
    
    Print(ts.c_str());
    
  } 
}
