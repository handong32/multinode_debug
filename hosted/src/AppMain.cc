//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <signal.h>
#include <thread>
#include <chrono>

#include <boost/filesystem.hpp>

#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

#include "EbbRTCoeffInit.h"

void func1(char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/AppMain.elf32";

  static ebbrt::Runtime runtime;
  static ebbrt::Context c(runtime);
  ebbrt::ContextActivation activation(c);

  int numNodes = 2;

  ebbrt::event_manager->Spawn([bindir, numNodes]() {
    EbbRTCoeffInit::Create(numNodes)
        .Then([bindir, numNodes](ebbrt::Future<EbbRTCoeffInitEbbRef> f) {
          EbbRTCoeffInitEbbRef ref = f.Get();

          std::cout << "#######################################EbbId: "
                    << ref->getEbbId() << std::endl;

          for (int i = 0; i < numNodes; i++) {
            std::cout << bindir.string() << std::endl;
            ebbrt::NodeAllocator::NodeDescriptor nd =
                ebbrt::node_allocator->AllocateNode(bindir.string(), 1, 1, 2);

            nd.NetworkId().Then([ref](
                ebbrt::Future<ebbrt::Messenger::NetworkId> f) {
              ebbrt::Messenger::NetworkId nid = f.Get();
              std::cout << nid.ToString() << std::endl;
              ref->addNid(nid);
            });
          }

          // waiting for all nodes to be initialized
          ref->waitNodes().Then([ref](ebbrt::Future<void> f) {
            f.Get();
            std::cout << "all nodes initialized" << std::endl;
            ebbrt::event_manager->Spawn([ref]() { ref->runJob(10000000); });
          });
        });
  });

  c.Deactivate();
  c.Run();
  c.Reset();

  std::cout << "Finished" << std::endl;
}

int main(int argc, char** argv) {
    func1(argv);
    return 0;
}
