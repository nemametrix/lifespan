#pragma once

#include "ns_threshold_movement_posture_analyzer.h"
#include "ns_hidden_markov_model_posture_analyzer.h"


class ns_threshold_and_hmm_posture_analyzer : public ns_analyzed_image_time_path_death_time_estimator{
public:
	ns_threshold_and_hmm_posture_analyzer(const ns_posture_analysis_model& p) : model(&p) {}

	ns_time_path_posture_movement_solution operator()(const ns_analyzed_image_time_path* path, std::ostream* debug_output_ = 0)const {
		std::vector<double > tmp_storage_1;
		std::vector<unsigned long > tmp_storage_2;
		return estimate_posture_movement_states(2, path, tmp_storage_1, tmp_storage_2, 0, debug_output_);
	}
	ns_time_path_posture_movement_solution operator()(const ns_analyzed_image_time_path* path, const bool fill_in_loglikelihood_timeseries, std::vector<double >& tmp_storage_1, std::vector<unsigned long >& tmp_storage_2, std::ostream* debug_output_ = 0)const {
		return estimate_posture_movement_states(2, path, tmp_storage_1, tmp_storage_2, 0, debug_output_);
	}
	ns_time_path_posture_movement_solution operator() (ns_analyzed_image_time_path* path, const bool fill_in_loglikelihood_timeseries, std::ostream* debug_output = 0)const {
		std::vector<double > tmp_storage_1;
		std::vector<unsigned long > tmp_storage_2;
		return estimate_posture_movement_states(2, path, tmp_storage_1, tmp_storage_2, path, debug_output);
	}

	ns_time_path_posture_movement_solution estimate_posture_movement_states(int software_value, const ns_analyzed_image_time_path* source_path, std::vector<double >& tmp_storage_1, std::vector<unsigned long >& tmp_storage_2, ns_analyzed_image_time_path* output_path = 0, std::ostream * debug_output = 0) const;

	unsigned long latest_possible_death_time(const ns_analyzed_image_time_path* path, const unsigned long last_observation_time) const;
	std::string software_version_number() const { return model->threshold_parameters.version_flag; }
private:
	ns_time_path_posture_movement_solution run(const ns_analyzed_image_time_path* path, std::ostream* debug_output = 0) const;
	const ns_posture_analysis_model * model;
};