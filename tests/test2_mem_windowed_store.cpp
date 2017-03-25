#include <cassert>
#include <kspp/state_stores/mem_windowed_store.h>

using namespace std::chrono_literals;

int main(int argc, char** argv) {
  // insert 3 check size
  kspp::mem_windowed_store<int32_t, std::string> store("", 100ms, 10);

  auto t0 = kspp::milliseconds_since_epoch();
  store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(0, "value0", t0));
  store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(1, "value1", t0 + 200));
  store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(2, "value2", t0 + 400));
  assert(store.size() == 3);

  // update existing key with new value
  {
    store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(2, "value2updated", t0 + 400));
    assert(store.size() == 3);
    auto record = store.get(2);
    assert(record != nullptr);
    assert(record->key == 2);
    assert(record->value != nullptr);
    assert(*record->value == "value2updated");
  }

  // update existing key with new value but old timestamp
  {
    store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(2, "to_old", t0 + 200));
    assert(store.size() == 3);
    auto record = store.get(2);
    assert(record != nullptr);
    assert(record->key == 2);
    assert(record->value != nullptr);
    assert(*record->value == "value2updated");
  }

  // delete existing key with to old timestamp
  {
    store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(2, nullptr, t0));
    assert(store.size() == 3);
    auto record = store.get(2);
    assert(record != nullptr);
    assert(record->key == 2);
    assert(record->value != nullptr);
    assert(*record->value == "value2updated");
  }

  // delete existing key with new timestamp
  {
    store.insert(std::make_shared<kspp::krecord<int32_t, std::string>>(2, nullptr, t0 + 700));
    assert(store.size() == 2);
    auto record = store.get(2);
    assert(record == nullptr);
  }

  // test garbage collection
  {
    store.garbage_collect(t0);
    assert(store.size() == 2);
    store.garbage_collect(t0 + 1000);
    assert(store.size() == 2);

    // only item 1 should be left
    store.garbage_collect(t0 + 1100);
    assert(store.size() == 1);
    auto record = store.get(1);
    assert(record != nullptr);
    assert(record->key == 1);
    assert(record->value != nullptr);
    assert(*record->value == "value1");

    // this should clear out item 2
    store.garbage_collect(t0 + 1300);
    assert(store.size() == 0);
  }
  return 0;
}

