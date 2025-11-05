/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "lib/e2/e2sm/e2sm_kpm/e2sm_kpm_du_meas_provider_impl.h"
#include "tests/unittests/e2/common/e2_test_helpers.h"
#include "srsran/ran/du_types.h"
#include "srsran/ran/sch/sch_mcs.h"
#include <gtest/gtest.h>

using namespace srsran;
using namespace asn1::e2sm;

static span<const e2sm_kpm_metric_t> e2sm_kpm_28_552_metrics = get_e2sm_kpm_28_552_metrics();
static span<const e2sm_kpm_metric_t> e2sm_kpm_oran_metrics   = get_e2sm_kpm_oran_metrics();

bool get_metric_definition(std::string metric_name, e2sm_kpm_metric_t& e2sm_kpm_metric_def)
{
  auto name_matches = [&metric_name](const e2sm_kpm_metric_t& x) {
    return (x.name == metric_name.c_str() or x.name == metric_name);
  };

  const auto* it = std::find_if(e2sm_kpm_28_552_metrics.begin(), e2sm_kpm_28_552_metrics.end(), name_matches);
  if (it != e2sm_kpm_28_552_metrics.end()) {
    e2sm_kpm_metric_def = *it;
    return true;
  }

  it = std::find_if(e2sm_kpm_oran_metrics.begin(), e2sm_kpm_oran_metrics.end(), name_matches);
  if (it != e2sm_kpm_oran_metrics.end()) {
    e2sm_kpm_metric_def = *it;
    return true;
  }

  return false;
}

static rlc_metrics generate_non_zero_rlc_metrics(uint32_t ue_idx, uint32_t bearer_id)
{
  rlc_metrics rlc_metric;
  rlc_metric.metrics_period        = std::chrono::milliseconds(1000);
  rlc_metric.ue_index              = static_cast<du_ue_index_t>(ue_idx);
  rlc_metric.rb_id                 = rb_id_t(drb_id_t(bearer_id));
  rlc_metric.rx.mode               = rlc_mode::am;
  rlc_metric.rx.num_pdus           = 5;
  rlc_metric.rx.num_pdu_bytes      = rlc_metric.rx.num_pdus * 1000;
  rlc_metric.rx.num_sdus           = 5;
  rlc_metric.rx.num_sdu_bytes      = rlc_metric.rx.num_sdus * 1000;
  rlc_metric.rx.num_lost_pdus      = 1;
  rlc_metric.rx.num_malformed_pdus = 1;
  rlc_metric.rx.sdu_latency_us     = 1000;

  rlc_metric.tx.tx_high.num_sdus                     = 10;
  rlc_metric.tx.tx_high.num_sdu_bytes                = rlc_metric.tx.tx_high.num_sdus * 1000;
  rlc_metric.tx.tx_high.num_dropped_sdus             = 1;
  rlc_metric.tx.tx_high.num_discarded_sdus           = 1;
  rlc_metric.tx.tx_high.num_discard_failures         = 1;
  rlc_metric.tx.tx_low.sum_sdu_latency_us            = 1000;
  rlc_metric.tx.tx_low.num_of_pulled_sdus            = 1;
  rlc_metric.tx.tx_low.num_pdus_no_segmentation      = 10;
  rlc_metric.tx.tx_low.num_pdu_bytes_no_segmentation = rlc_metric.tx.tx_low.num_pdus_no_segmentation * 1000;

  rlc_metric.tx.tx_low.mode_specific = rlc_am_tx_metrics_lower{};
  auto& am                           = std::get<rlc_am_tx_metrics_lower>(rlc_metric.tx.tx_low.mode_specific);
  am.num_pdus_with_segmentation      = 2;
  am.num_pdu_bytes_with_segmentation = am.num_pdus_with_segmentation * 1000;

  return rlc_metric;
}

static scheduler_cell_metrics generate_non_zero_sched_metrics()
{
  scheduler_cell_metrics sched_metric;
  sched_metric.nof_prbs            = 52;
  sched_metric.nof_dl_slots        = 14;
  sched_metric.nof_ul_slots        = 14;
  sched_metric.nof_prach_preambles = 10;

  scheduler_ue_metrics ue_metrics;
  ue_metrics.ue_index            = to_du_ue_index(0);
  ue_metrics.pci                 = 1;
  ue_metrics.rnti                = static_cast<rnti_t>(0x1000 + 1);
  ue_metrics.dl_mcs              = sch_mcs_index{15};  // Set MCS index for DL
  ue_metrics.tot_pdsch_prbs_used = 1200;
  ue_metrics.tot_pusch_prbs_used = 1200;
  ue_metrics.dl_brate_kbps       = 1000.0;  // Add non-zero DL bit rate
  ue_metrics.dl_nof_ok           = 100;     // Add non-zero DL OK count
  ue_metrics.dl_nof_nok          = 5;       // Add non-zero DL NOK count
  ue_metrics.ul_brate_kbps       = 800.0;   // Add non-zero UL bit rate
  ue_metrics.ul_nof_ok           = 95;      // Add non-zero UL OK count
  ue_metrics.ul_nof_nok          = 3;       // Add non-zero UL NOK count
  ue_metrics.avg_crc_delay_ms    = 100;
  ue_metrics.pusch_snr_db        = 10;
  ue_metrics.ul_mcs              = sch_mcs_index{12};  // Set MCS index for UL
  ue_metrics.last_phr            = 20;      // Set PHR value
  ue_metrics.dl_bs               = 1234;
  ue_metrics.bsr                 = 5678;  // Add non-zero BSR value
  for (auto i = 0; i < 10; i++) {
    ue_metrics.cqi_stats.update(i);
  }
  // Add DL RI statistics
  for (auto i = 1; i <= 4; i++) {
    ue_metrics.dl_ri_stats.update(i);
  }
  // Add UL RI statistics  
  for (auto i = 1; i <= 4; i++) {
    ue_metrics.ul_ri_stats.update(i);
  }
  // Add TA statistics
  for (auto i = 0; i < 5; i++) {
    ue_metrics.ta_stats.update(0.1f * i);
  }
  sched_metric.ue_metrics.push_back(ue_metrics);

  return sched_metric;
}

class dummy_e2_du_metrics_notifier : public e2_du_metrics_notifier, public e2_du_metrics_interface
{
public:
  void report_metrics(const scheduler_cell_metrics& metrics) override
  {
    if (e2_meas_provider) {
      e2_meas_provider->report_metrics(metrics);
    }
  }

  void report_metrics(const rlc_metrics& metrics) override
  {
    if (e2_meas_provider) {
      e2_meas_provider->report_metrics(metrics);
    }
  }

  void connect_e2_du_meas_provider(std::unique_ptr<e2_du_metrics_notifier> meas_provider) override {}

  void connect_e2_du_meas_provider(e2_du_metrics_notifier* meas_provider) { e2_meas_provider = meas_provider; }

private:
  e2_du_metrics_notifier* e2_meas_provider;
};

class e2sm_kpm_meas_provider_metrics_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    srslog::fetch_basic_logger("TEST").set_level(srslog::basic_levels::debug);
    srslog::init();
    f1ap_ue_id_mapper = std::make_unique<dummy_f1ap_ue_id_translator>();
    du_meas_provider  = std::make_unique<e2sm_kpm_du_meas_provider_impl>(*f1ap_ue_id_mapper);
    metrics           = std::make_unique<dummy_e2_du_metrics_notifier>();
    metrics->connect_e2_du_meas_provider(du_meas_provider.get());
  }

  void TearDown() override
  {
    // Flush logger after each test.
    srslog::flush();
  }

  std::unique_ptr<dummy_e2_du_metrics_notifier>   metrics;
  std::unique_ptr<dummy_f1ap_ue_id_translator>    f1ap_ue_id_mapper;
  std::unique_ptr<e2sm_kpm_du_meas_provider_impl> du_meas_provider;
  srslog::basic_logger&                           test_logger = srslog::fetch_basic_logger("TEST");
};

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_supported_metrics_are_supported)
{
  e2sm_kpm_metric_level_enum metric_level;
  meas_type_c                meas_type;
  meas_label_s               meas_label;
  meas_label.no_label_present         = true;
  meas_label.no_label                 = meas_label_s::no_label_e_::true_value;
  bool                     cell_scope = false;
  std::vector<std::string> supported_metrics;
  bool                     metric_supported = false;

  // E2-NODE-LEVEL metrics
  metric_level      = E2_NODE_LEVEL;
  supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);
    metric_supported = du_meas_provider->is_metric_supported(meas_type, meas_label, metric_level, cell_scope);
    ASSERT_TRUE(metric_supported) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                                  << " returned as supported but not supported ";
  }

  // UE-LEVEL metrics
  metric_level      = UE_LEVEL;
  supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);
    metric_supported = du_meas_provider->is_metric_supported(meas_type, meas_label, metric_level, cell_scope);
    ASSERT_TRUE(metric_supported) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                                  << " returned as supported but not supported ";
  }
}

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_return_e2_level_metric_with_no_measurements)
{
  // Metrics that should return no_value when no measurements are present
  std::vector<std::string> no_value_metrics = {
      "DRB.AirIfDelayUl", "DRB.RlcSduDelayDl", "DRB.RlcDelayUl", "DRB.RlcPacketDropRateDl", "DRB.RlcSduDelayDl"};

  // E2-NODE-LEVEL metrics have to be returned even if 0 or NAN
  e2sm_kpm_metric_level_enum       metric_level = E2_NODE_LEVEL;
  meas_type_c                      meas_type;
  std::optional<asn1::e2sm::cgi_c> cell_global_id = {};
  e2sm_kpm_metric_t                e2sm_kpm_metric_definition;

  label_info_list_l label_info_list;
  label_info_item_s label_info_item           = {};
  label_info_item.meas_label.no_label_present = true;
  label_info_item.meas_label.no_label         = meas_label_s::no_label_e_::true_value;
  label_info_list.push_back(label_info_item);

  std::vector<meas_record_item_c> meas_records_items;

  std::vector<std::string> supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);
    meas_records_items.clear();

    // Get measurements
    du_meas_provider->get_meas_data(meas_type, label_info_list, {}, cell_global_id, meas_records_items);

    // If no measurements returned, check if metric is expected to be empty
    if (meas_records_items.empty()) {
      bool should_be_empty =
          (std::find(no_value_metrics.begin(), no_value_metrics.end(), metric) != no_value_metrics.end());
      ASSERT_TRUE(should_be_empty) << "Metric: " << metric << " at level " << e2sm_kpm_scope_2_str(metric_level)
                                   << " returned no measurements but was expected to return a record.";
      continue; // skip further checks
    }

    // At this point, meas_records_items has at least one element
    ASSERT_EQ(meas_records_items.size(), 1) << "Metric: " << metric << " returned unexpected number of measurements";

    // Check no_value metrics
    bool found_no_value =
        (std::find(no_value_metrics.begin(), no_value_metrics.end(), metric) != no_value_metrics.end());
    if (found_no_value) {
      ASSERT_EQ(meas_records_items[0].type(), meas_record_item_c::types::no_value)
          << "Metric: " << metric << " expected to return no_value when no measurements available";
      continue;
    }

    // Check metric type matches definition
    if (get_metric_definition(metric, e2sm_kpm_metric_definition)) {
      if (e2sm_kpm_metric_definition.data_type == e2sm_kpm_metric_dtype_t::INTEGER) {
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::integer)
            << "Metric: " << e2sm_kpm_metric_definition.name << " should be integer type";
      } else {
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::real)
            << "Metric: " << e2sm_kpm_metric_definition.name << " should be real type";
      }
    }

    // Validate value
    switch (meas_records_items[0].type()) {
      case meas_record_item_c::types::integer:
        ASSERT_EQ(meas_records_items[0].integer(), 0)
            << "Metric: " << metric << " should return 0 when no measurements";
        break;
      case meas_record_item_c::types::real:
        ASSERT_FLOAT_EQ(meas_records_items[0].real().value, 0.0)
            << "Metric: " << metric << " should return 0.0 when no measurements";
        break;
      default:
        FAIL() << "Metric: " << metric << " returned unexpected type: " << meas_records_items[0].type().value;
        break;
    }
  }
}

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_return_e2_level_metric_with_with_measurements)
{
  // E2-NODE-LEVEL metrics have to be always returned, even if 0 or NAN
  e2sm_kpm_metric_level_enum       metric_level = E2_NODE_LEVEL;
  meas_type_c                      meas_type;
  std::optional<asn1::e2sm::cgi_c> cell_global_id = {};
  e2sm_kpm_metric_t                e2sm_kpm_metric_definition;

  label_info_list_l label_info_list;
  label_info_item_s label_info_item           = {};
  label_info_item.meas_label.no_label_present = true;
  label_info_item.meas_label.no_label         = meas_label_s::no_label_e_::true_value;
  label_info_list.push_back(label_info_item);
  std::vector<meas_record_item_c> meas_records_items;

  // Fill e2sm-kpm measurements provider with RLC and SCHED metrics.
  // Generate dummy metrics that will generate non-zero e2sm-kpm metric records.
  rlc_metrics rlc_metric = generate_non_zero_rlc_metrics(0, 1);
  metrics->report_metrics(rlc_metric);
  scheduler_cell_metrics sched_metrics = generate_non_zero_sched_metrics();
  metrics->report_metrics(sched_metrics);

  std::vector<std::string> supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);

    meas_records_items.clear();
    du_meas_provider->get_meas_data(meas_type, label_info_list, {}, cell_global_id, meas_records_items);

    ASSERT_TRUE(meas_records_items.size() == 1)
        << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
        << " returned no measurements (size=" << meas_records_items.size() << ")";

    if (get_metric_definition(metric, e2sm_kpm_metric_definition)) {
      if (e2sm_kpm_metric_definition.data_type == e2sm_kpm_metric_dtype_t::INTEGER) {
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::integer)
            << "Metric: " << e2sm_kpm_metric_definition.name.c_str() << " should return record of the integer type.";
      } else {
        // e2sm_kpm_metric_dtype_t::REAL
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::real)
            << "Metric: " << e2sm_kpm_metric_definition.name.c_str() << " should return record of the real type.";
      }
    }

    switch (meas_records_items[0].type()) {
      case meas_record_item_c::types::integer:
        ASSERT_NE(meas_records_items[0].integer(), 0)
            << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level);
        break;
      case meas_record_item_c::types::real:
        ASSERT_NE(meas_records_items[0].real().value, 0.0)
            << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level);
        break;
      default:
        printf("%s type: %i\n", metric.c_str(), meas_records_items[0].type().value);
        ASSERT_TRUE(false) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                           << " returned a record with wrong type.";
        break;
    }
  }
}
