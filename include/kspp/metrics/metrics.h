#include <string>
#include <chrono>
#include <cmath>
#include <functional>
#include <prometheus/registry.h>
#pragma once

#define KSPP_KEY_TYPE_TAG "key_type"
#define KSPP_VALUE_TYPE_TAG "value_type"
#define KSPP_PROCESSOR_TYPE_TAG "processor_type"
#define KSPP_DESTINATION_HOST   "destination_host"
#define KSPP_PARTITION_TAG "partition"
#define KSPP_TOPIC_TAG "topic"

namespace kspp {
  struct metric {
    enum mtype { RATE, COUNT, GAUGE, COUNTER, TIMESTAMP, SUMMARY, HISTOGRAM }; // http://metrics20.org/spec/ and some prometheus

    metric(std::string what, mtype mt, std::string unit)
        : _name("kspp_" + what) {
      switch (mt) {
        case RATE: add_tag("mtype", "rate"); break;
        case COUNT: add_tag("mtype", "count"); break;
        case GAUGE: add_tag("mtype", "gauge"); break;
        case COUNTER: add_tag("mtype", "counter"); break;
        case TIMESTAMP: add_tag("mtype", "timestamp"); break;
        case SUMMARY: add_tag("mtype", "summary"); break;
        case HISTOGRAM: add_tag("mtype", "histogram"); break;
      }
      add_tag("unit", unit);
    }

    virtual double value() const = 0;

    inline std::string name() const {
      return _name;
    }

    virtual void finalize_tags(std::shared_ptr<prometheus::Registry> registry)=0;

    void add_tag(std::string key, std::string val)
    {
      _real_tags[key]=val;
    }

    std::string _name; // what
    std::map<std::string, std::string> _real_tags;
  };

  struct metric_counter : public metric {
    metric_counter(std::string what, std::string unit)
        : metric(what, COUNTER, unit)
        ,  _counter(nullptr) {
    }

    void finalize_tags(std::shared_ptr<prometheus::Registry> registry) override
    {
      auto& counter_family = prometheus::BuildCounter().Name(_name).Register(*registry);
      _counter = &counter_family.Add(_real_tags);
    }

    virtual double value() const {
      return _counter->Value();
    }

    inline metric_counter &operator++() {
      _counter->Increment();
      return *this;
    }

    inline metric_counter &operator+=(double v) {
      _counter->Increment(v);
      return *this;
    }

    prometheus::Counter* _counter;
  };

  struct metric_gauge : public metric {
    metric_gauge(std::string what, std::string unit)
        : metric(what, GAUGE, unit)
        ,  _gauge(nullptr) {
    }

    void finalize_tags(std::shared_ptr<prometheus::Registry> registry) override
    {
      auto& family = prometheus::BuildGauge().Name(_name).Register(*registry);
      _gauge = &family.Add(_real_tags);
    }

    void set(double v) {
      _gauge->Set(v);
    }

    virtual double value() const {
     _gauge->Value();
    }

    void clear() {
      _gauge->Set(0);
    }

    prometheus::Gauge* _gauge;
  };

  struct metric_streaming_lag : public metric {
    metric_streaming_lag()
        : metric("streaming_lag", GAUGE, "ms")
        ,  _gauge(nullptr) {

    }

    void finalize_tags(std::shared_ptr<prometheus::Registry> registry) override
    {
      auto& family = prometheus::BuildGauge().Name(_name).Register(*registry);
      _gauge = &family.Add(_real_tags);
    }

    inline void add_event_time(int64_t tick, int64_t event_time) {
      if (event_time > 0)
        _gauge->Set(tick - event_time);
      else
        _gauge->Set(-1.0);
    }

    virtual double value() const {
      _gauge->Value();
    }

  private:
    prometheus::Gauge* _gauge;
  };

  struct metric_summary : public metric {
    metric_summary(std::string what, std::string unit, const std::vector<float>& quantiles={0.99})
        : metric(what, SUMMARY, unit)
        ,  _quantiles(quantiles)
        ,  _summary(nullptr) {
      add_tag("window", "60s");
    }

    void finalize_tags(std::shared_ptr<prometheus::Registry> registry) override
    {
        std::vector<prometheus::detail::CKMSQuantiles::Quantile> q;
      for (auto i : _quantiles)
        q.emplace_back(i, 0.05);
      auto& family = prometheus::BuildSummary().Name(_name).Register(*registry);
      _summary = &family.Add(_real_tags, q, std::chrono::seconds{600}, 5);
    }

    inline void observe(double v) {
      _summary->Observe(v);
    }

    virtual double value() const {
      return NAN;
    }

  private:
    const std::vector<float> _quantiles;
    prometheus::Summary* _summary;
  };


  struct metric_histogram : public metric {
    metric_histogram(std::string what, std::string unit, const std::vector<double>& buckets)
        : metric(what, HISTOGRAM, unit)
        ,  _buckets(buckets)
        ,  _histgram(nullptr) {
    }

    void finalize_tags(std::shared_ptr<prometheus::Registry> registry) override
    {
      auto& family = prometheus::BuildHistogram().Name(_name).Register(*registry);
      _histgram = &family.Add(_real_tags, _buckets);
    }

    inline void observe(double v) {
      _histgram->Observe(v);
    }

    virtual double value() const {
      return NAN;
    }

  private:
    const std::vector<double> _buckets;
    prometheus::Histogram* _histgram;
  };
}
