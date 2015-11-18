#include "EbbRTCoeffInit.h"

EBBRT_PUBLISH_TYPE(, EbbRTCoeffInit);

struct membuf : std::streambuf {
  membuf(char* begin, char* end) { this->setg(begin, begin, end); }
};

void EbbRTCoeffInit::Print(ebbrt::Messenger::NetworkId nid, const char* str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);

  std::cout << "runJob2(): length of sent iobuf: "
            << buf->ComputeChainDataLength() << " bytes" << std::endl;

  SendMessage(nid, std::move(buf));
}

void EbbRTCoeffInit::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                                    std::unique_ptr<ebbrt::IOBuf>&& buffer) {
  /***********************************
   * std::string(reinterpret_cast<const char*> buffer->Data(),
   *buffer->Length())
   *
   * Using the above code, I had to first do strcmp(output.c_str(), ...) to
   *ensure
   * it matched the input string.
   * Direct comparison using "==" seems to be working when I don't pass in the
   *Length() as second arg
   **********************************/
  auto output = std::string(reinterpret_cast<const char*>(buffer->Data()));
  std::cout << "Received ip: " << nid.ToString() << std::endl;

  if (output[0] == 'G') {
    std::cout << "FE received D" << std::endl;
    ebbrt::IOBuf::DataPointer dp = buffer->GetDataPointer();
    char* t = (char*)(dp.Get(buffer->ComputeChainDataLength()));
    membuf sb{t + 2, t + buffer->ComputeChainDataLength()};
    std::istream stream{&sb};
    std::vector<int> rvec;

    boost::archive::text_iarchive ia(stream);
    ia& rvec;

    std::cout << "Parsing it back, received: "
              << buffer->ComputeChainDataLength() << " bytes" << std::endl;
    std::cout << "rvec.size() = " << rvec.size() << std::endl;
    
    recv_counter++;
    std::cout << "recv_counter++ " << recv_counter << std::endl;

    if (recv_counter == numNodes) 
    {
	std::cout << "recv_counter == numNodes" << recv_counter << std::endl;
	// reactivate context
	ebbrt::event_manager->ActivateContext(std::move(*emec));
    }
  }
}

void EbbRTCoeffInit::runJob(int size) {
  // get the event manager context and save it
  ebbrt::EventManager::EventContext context;

  std::vector<int> vec;
  for(int i = 0; i < size; i ++)
  {
      vec.push_back(i);
  }

  for (int i = 0; i < (int)nids.size(); i++) {
    int start, end, factor;
    factor = (int)ceil(vec.size() / (float)numNodes);
    start = i * factor;
    end = i * factor + factor;
    end = ((size_t)end > vec.size()) ? vec.size() : end;

    std::ostringstream ofs;
    boost::archive::text_oarchive oa(ofs);

    oa& start& end;

    for (int j = start; j < end; j++) {
      oa& vec[j];
    }

    std::string ts = "G " + ofs.str();

    std::cout << "Sending to .. " << nids[i].ToString() << " " << start << " "
              << end << std::endl;
    Print(nids[i], ts.c_str());
  }

  emec = &context;
  ebbrt::event_manager->SaveContext(*emec);

  std::cout << "Received back " << std::endl;
  ebbrt::active_context->io_service_.stop();
}
