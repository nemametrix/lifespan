#include "ns_time_path_image_analyzer.h"
#include "ns_graph.h"
#include "ns_xml.h"
#include "ns_image_tools.h"
#include "ctmf.h"
using namespace std;

#define NS_MARGIN_BACKGROUND 0
#define NS_DO_SUBPIXEL_REGISTRATION
#define NS_SUBPIXEL_REGISTRATION_CORSE ns_vector_2d(.25,.25)
#define NS_SUBPIXEL_REGISTRATION_FINE ns_vector_2d(.125,.125)

const bool ns_normalize_individual_movement_timeseries_to_median(false);
const bool ns_skip_low_density_paths(false);

//#define NS_OUTPUT_ALGINMENT_DEBUG


ns_analyzed_image_time_path_element_measurements operator+(const ns_analyzed_image_time_path_element_measurements & a,const ns_analyzed_image_time_path_element_measurements & b){
	ns_analyzed_image_time_path_element_measurements s;

	s.interframe_time_scaled_movement_sum	=a.	interframe_time_scaled_movement_sum		+b.interframe_time_scaled_movement_sum;
	s.movement_alternate_worm_sum			=a.	movement_alternate_worm_sum				+b.movement_alternate_worm_sum;
//	s.stationary_sum						=a.	stationary_sum							+b.stationary_sum;
//	s.stationary_change_sum					=a.	stationary_change_sum					+b.stationary_change_sum;
	s.total_worm_area							=a.	total_worm_area							+b.total_worm_area;
	s.total_intensity_within_worm_area		=a.	total_intensity_within_worm_area		+b.total_intensity_within_worm_area;
	s.total_region_area						=a.	total_region_area						+b.total_region_area;
	s.total_intensity_within_region			=a.	total_intensity_within_region			+b.total_intensity_within_region;
	s.total_alternate_worm_area				=a.	total_alternate_worm_area				+b.total_alternate_worm_area;
	s.total_intensity_within_alternate_worm	=a.	total_intensity_within_alternate_worm	+b.total_intensity_within_alternate_worm;
	s.change_in_average_normalized_worm_intensity = a.change_in_average_normalized_worm_intensity + b.change_in_average_normalized_worm_intensity;
	s.change_in_total_worm_intensity = a.change_in_total_worm_intensity + b.change_in_total_worm_intensity;
	s.change_in_average_region_intensity = a.change_in_average_region_intensity + b.change_in_average_region_intensity;
	
	s.registration_displacement= a.registration_displacement + b.registration_displacement;

	s.mean_intensity_within_worm_area			=a.	mean_intensity_within_worm_area			+b.mean_intensity_within_worm_area;
	s.mean_intensity_within_region			=a.	mean_intensity_within_region			+b.mean_intensity_within_region;
	s.mean_intensity_within_alternate_worm	=a.	mean_intensity_within_alternate_worm	+b.mean_intensity_within_alternate_worm;
	return s;
}

void ns_analyzed_image_time_path_element_measurements::calculate_means(){
	if (total_worm_area != 0)
		mean_intensity_within_worm_area = total_intensity_within_worm_area/(double)total_worm_area;
	else mean_intensity_within_worm_area = 0;
	if (total_region_area != 0)
		mean_intensity_within_region= total_intensity_within_region/(double)total_region_area;
	else mean_intensity_within_region = 0;
	if (total_alternate_worm_area != 0)
		mean_intensity_within_alternate_worm= total_intensity_within_alternate_worm/(double)total_alternate_worm_area;
	else mean_intensity_within_alternate_worm = 0;
}

ns_analyzed_image_time_path_element_measurements operator/(const ns_analyzed_image_time_path_element_measurements & a,const int & d){
	ns_analyzed_image_time_path_element_measurements s(a);
	s.interframe_time_scaled_movement_sum/=d;
	s.movement_alternate_worm_sum/=d;
	//s.stationary_sum/=d;
	s.change_in_average_normalized_worm_intensity/=d;
	s.change_in_total_worm_intensity/=d;
	s.change_in_average_region_intensity/=d;
	s.total_worm_area/=d;
	s.total_intensity_within_worm_area/=d;
	s.total_region_area/=d;
	s.total_intensity_within_region/=d;
	s.total_alternate_worm_area/=d;
	s.total_intensity_within_alternate_worm/=d;

	s.registration_displacement = s.registration_displacement/d;

	s.mean_intensity_within_worm_area/=d;
	s.mean_intensity_within_region/=d;
	s.mean_intensity_within_alternate_worm/=d;
	return s;
}

void ns_analyzed_image_time_path_element_measurements::square(){
	unnormalized_movement_sum							*=	unnormalized_movement_sum;
	movement_alternate_worm_sum				*=	movement_alternate_worm_sum;
//	stationary_sum							*=	stationary_sum;
	change_in_average_normalized_worm_intensity					*=	change_in_average_normalized_worm_intensity;
	change_in_total_worm_intensity					*=	change_in_total_worm_intensity;
	change_in_average_region_intensity					*=	change_in_average_region_intensity;
	total_worm_area							*=	total_worm_area;
	total_intensity_within_worm_area		*=	total_intensity_within_worm_area;
	total_region_area						*=	total_region_area;
	total_intensity_within_region			*=	total_intensity_within_region;
	total_alternate_worm_area				*=	total_alternate_worm_area;
	total_intensity_within_alternate_worm	*=	total_intensity_within_alternate_worm;

	registration_displacement.x*= registration_displacement.x;
	registration_displacement.y*= registration_displacement.y;

	mean_intensity_within_worm_area			*=	mean_intensity_within_worm_area;
	mean_intensity_within_region			*=	mean_intensity_within_region;
	mean_intensity_within_alternate_worm	*=	mean_intensity_within_alternate_worm;
}
void ns_analyzed_image_time_path_element_measurements::square_root(){
	unnormalized_movement_sum						=sqrt((double)		unnormalized_movement_sum);
	movement_alternate_worm_sum			=sqrt((double)		movement_alternate_worm_sum);
	this->change_in_average_region_intensity						=sqrt((double)		this->change_in_average_region_intensity);
	this->change_in_average_normalized_worm_intensity				=sqrt((double)		this->change_in_average_normalized_worm_intensity);
	this->change_in_total_worm_intensity				=sqrt((double)		this->change_in_total_worm_intensity);
	total_worm_area						=sqrt((double)		total_worm_area);
	total_intensity_within_worm_area	=sqrt((double)		total_intensity_within_worm_area);
	total_region_area					=sqrt((double)		total_region_area);
	total_intensity_within_region		=sqrt((double)		total_intensity_within_region);
	total_alternate_worm_area			=sqrt((double)		total_alternate_worm_area);
	total_intensity_within_alternate_worm=sqrt((double)		total_intensity_within_alternate_worm);

	registration_displacement.x = sqrt(registration_displacement.x);
	registration_displacement.y = sqrt(registration_displacement.y);

	mean_intensity_within_worm_area		=sqrt(		mean_intensity_within_worm_area);
	mean_intensity_within_region		=sqrt(		mean_intensity_within_region);
	mean_intensity_within_alternate_worm=sqrt(		mean_intensity_within_alternate_worm);
}

void ns_analyzed_image_time_path_element_measurements::zero(){
	unnormalized_movement_sum=0;
	movement_alternate_worm_sum=0;
	//stationary_sum=0;
//	stationary_change_sum=0;
	this->change_in_average_region_intensity = 0;
	this->change_in_average_normalized_worm_intensity = 0;
	this->change_in_total_worm_intensity = 0;
	total_worm_area=0;
	total_intensity_within_worm_area=0;
	total_region_area=0;
	total_intensity_within_region=0;
	total_alternate_worm_area=0;
	total_intensity_within_alternate_worm=0;

	registration_displacement = ns_vector_2d(0,0);

	mean_intensity_within_worm_area=0;
	mean_intensity_within_region=0;
	mean_intensity_within_alternate_worm=0;
}

class ns_chunk_generator{
public:
	ns_chunk_generator(const unsigned long chunk_size_, const ns_analyzed_image_time_path & path_):path(&path_),chunk_size(chunk_size_),current_chunk(0,0){}
	
	
	void setup_first_chuck_for_forwards_registration(){
		setup_first_chunk(false);
		if (current_chunk.start_i > -1 &&  (path->element_count() - current_chunk.start_i) <  ns_analyzed_image_time_path::alignment_time_kernel_width){
				current_chunk.start_i = current_chunk.stop_i = (path->element_count()-ns_analyzed_image_time_path::alignment_time_kernel_width);
		}
		first_chunk_ = current_chunk;
		current_chunk.processing_direction = ns_analyzed_time_image_chunk::ns_forward;
	}
	void setup_first_chuck_for_backwards_registration(){
		setup_first_chunk(true);
		//long chunk_length = current_chunk.start_i - current_chunk.stop_i;
		//if there isn't enough data to support the registration,
		//backwards from data collected before the onset of stationarity,
		//buffer it with data collected after the onset stationarity
		if (current_chunk.start_i > -1 &&  current_chunk.start_i <  ns_analyzed_image_time_path::alignment_time_kernel_width){

				current_chunk.start_i = current_chunk.stop_i = (ns_analyzed_image_time_path::alignment_time_kernel_width)-1;
		}
		first_chunk_ = current_chunk;
		current_chunk.processing_direction = ns_analyzed_time_image_chunk::ns_backward;
	}

	bool update_and_check_for_new_chunk( ns_analyzed_time_image_chunk & new_chunk){
		
		current_chunk.processing_direction = ns_analyzed_time_image_chunk::ns_forward;
		for (;current_chunk.stop_i < path->element_count() && path->element(current_chunk.stop_i).path_aligned_image_is_loaded(); 
			current_chunk.stop_i++);
		const unsigned long cur_size(current_chunk.stop_i-current_chunk.start_i);
		if ( cur_size >= chunk_size ||
			(cur_size > 0 && current_chunk.stop_i == path->element_count())){
			
				new_chunk = current_chunk;
				current_chunk.start_i = current_chunk.stop_i;
				return true;
		}
		return false;
	}

	bool backwards_update_and_check_for_new_chunk(ns_analyzed_time_image_chunk & new_chunk){

		new_chunk.processing_direction= ns_analyzed_time_image_chunk::ns_backward;
		for (;current_chunk.stop_i >= 0 && path->element(current_chunk.stop_i).path_aligned_image_is_loaded(); current_chunk.stop_i--);

		const unsigned long cur_size(current_chunk.start_i-current_chunk.stop_i);
		//if (cur_size < 7 && cur_size > 0)
		//	cerr << "MA";
		if (current_chunk.start_i > -1 && 
			(cur_size >= chunk_size ||
			current_chunk.stop_i == -1)){
				new_chunk = current_chunk;
				//mark the current chunk (which will be referenced in the next iteration) as starting at the first unprocessed position (the stop of the new chunk)
				current_chunk.start_i = current_chunk.stop_i;
				return true;
		}
		return false;
	}
	bool no_more_chunks(){return current_chunk.stop_i == path->element_count();}
	
	const ns_analyzed_time_image_chunk & first_chunk() const {return first_chunk_;}
private:
	unsigned long chunk_size;
	long number_of_previous_elements;
	ns_analyzed_time_image_chunk current_chunk;
	const ns_analyzed_image_time_path * path;
	ns_analyzed_time_image_chunk first_chunk_;

	void setup_first_chunk(bool crop_zero){
	//	for (number_of_previous_elements = 0; number_of_previous_elements < path->element_count(); number_of_previous_elements++){
	//		if (!path->element(number_of_previous_elements).element_before_fast_movement_cessation)
	//			break;
	//	}
		number_of_previous_elements = path->first_stationary_timepoint();
		if (number_of_previous_elements >= path->element_count())
			throw ns_ex("No non-previous worm images found!");
		if (crop_zero && number_of_previous_elements == 0)	//don't do anything if there are no previous elements
			current_chunk.stop_i = current_chunk.start_i = -1; 
		else current_chunk.stop_i = current_chunk.start_i = number_of_previous_elements;  //include first two frames if there are elements
	}
};
void ns_time_path_image_movement_analyzer::delete_from_db(const unsigned long region_id,ns_sql & sql){
	sql << "DELETE FROM path_data WHERE region_id = " << region_id;
	sql.send_query();
	sql << "UPDATE sample_region_image_info SET path_movement_images_are_cached=0 WHERE id = " << region_id;
	sql.send_query();
}

void ns_movement_posture_visualization_summary::to_xml(std::string & text){
	ns_xml_simple_writer xml;
	xml.add_tag("rid",region_id);
	xml.add_tag("fn",frame_number);
	xml.add_tag("afn",alignment_frame_number);
	for (unsigned int i = 0; i < worms.size(); i++){
		xml.start_group("w");
		xml.add_tag("swp",ns_xml_simple_writer::format_pair(worms[i].worm_in_source_image.position.x,worms[i].worm_in_source_image.position.y));
		xml.add_tag("swz",ns_xml_simple_writer::format_pair(worms[i].worm_in_source_image.size.x,worms[i].worm_in_source_image.size.y));

		xml.add_tag("spp",ns_xml_simple_writer::format_pair(worms[i].path_in_source_image.position.x,worms[i].path_in_source_image.position.y));
		xml.add_tag("spz",ns_xml_simple_writer::format_pair(worms[i].path_in_source_image.size.x,worms[i].path_in_source_image.size.y));

		xml.add_tag("vp",ns_xml_simple_writer::format_pair(worms[i].path_in_visualization.position.x,worms[i].path_in_visualization.position.y));
		xml.add_tag("vz",ns_xml_simple_writer::format_pair(worms[i].path_in_visualization.size.x,worms[i].path_in_visualization.size.y));

		xml.add_tag("mp",ns_xml_simple_writer::format_pair(worms[i].metadata_in_visualizationA.position.x,worms[i].metadata_in_visualizationA.position.y));
		xml.add_tag("mz",ns_xml_simple_writer::format_pair(worms[i].metadata_in_visualizationA.size.x,worms[i].metadata_in_visualizationA.size.y));

		xml.add_tag("pi",worms[i].stationary_path_id.path_id);
		xml.add_tag("gi",worms[i].stationary_path_id.group_id);
		xml.add_tag("gt",worms[i].stationary_path_id.detection_set_id);
		if (worms[i].path_time.period_start_was_not_observed)
			xml.add_tag("ps",-1);
		else xml.add_tag("ps",worms[i].path_time.period_start);
		if (worms[i].path_time.period_end_was_not_observed)
			xml.add_tag("pf",worms[i].path_time.period_end);
		else xml.add_tag("pf",worms[i].path_time.period_end);
		xml.add_tag("tt",worms[i].image_time);
		xml.end_group();
	}
	text = xml.result();
}
inline ns_vector_2i ns_get_integer_pair(const std::string & s){
	std::string::size_type t(s.find(","));
	if (t == std::string::npos)
		throw ns_ex("Could not find pair information in '") << s << "'";
	else return ns_vector_2i(atol(s.substr(0,t).c_str()),(atol(s.substr(t+1,std::string::npos).c_str())));
}
void ns_movement_posture_visualization_summary::from_xml(const std::string & text){
	ns_xml_simple_object_reader o;
	o.from_string(text);
	worms.resize(0);
	if (o.objects.size() == 0) return;
	worms.reserve(o.objects.size()-1);
	frame_number = 0;
	alignment_frame_number = 0;
	try{
		for (unsigned int i = 0; i < o.objects.size(); i++){
			if (o.objects[i].name == "rid"){
				region_id = atol(o.objects[i].value.c_str());
				continue;
			}
			if (o.objects[i].name == "fn"){
				frame_number = atol(o.objects[i].value.c_str());
				continue;
			}
			if (o.objects[i].name == "afn"){
				alignment_frame_number = atol(o.objects[i].value.c_str());
				continue;
			}
			else if (o.objects[i].name!= "w")
				throw ns_ex("Unknown posture visualization summary tag: ") << o.objects[i].name;
			string::size_type s = worms.size();
			worms.resize(s+1);
			
			worms[s].worm_in_source_image.position = ns_get_integer_pair(o.objects[i].tag("swp"));
			worms[s].worm_in_source_image.size = ns_get_integer_pair(o.objects[i].tag("swz"));
			worms[s].path_in_source_image.position = ns_get_integer_pair(o.objects[i].tag("spp"));
			worms[s].path_in_source_image.size = ns_get_integer_pair(o.objects[i].tag("spz"));
			worms[s].path_in_visualization.position = ns_get_integer_pair(o.objects[i].tag("vp"));
			worms[s].path_in_visualization.size = ns_get_integer_pair(o.objects[i].tag("vz"));
			worms[s].metadata_in_visualizationA.position = ns_get_integer_pair(o.objects[i].tag("mp"));
			worms[s].metadata_in_visualizationA.size = ns_get_integer_pair(o.objects[i].tag("mz"));
			worms[s].stationary_path_id.path_id = atol(o.objects[i].tag("pi").c_str());
			worms[s].stationary_path_id.group_id = atol(o.objects[i].tag("gi").c_str());
			worms[s].stationary_path_id.detection_set_id = atol(o.objects[i].tag("gt").c_str());
			if (o.objects[i].tag("ps") == "-1")
				worms[s].path_time.period_start_was_not_observed = true;
			else 
				worms[s].path_time.period_start = atol(o.objects[i].tag("ps").c_str());
			if (o.objects[i].tag("pf") == "-1")
				worms[s].path_time.period_end= atol(o.objects[i].tag("pf").c_str());
			else worms[s].path_time.period_end= atol(o.objects[i].tag("pf").c_str());
			worms[s].image_time = atol(o.objects[i].tag("tt").c_str());
		}
	}
	catch(ns_ex & ex){
		worms.clear();
		throw ex;
	}
}
ns_64_bit ns_largest_delta_subdivision(const std::vector<ns_64_bit> & v,const int sub_div){
	const unsigned long d_i(ceil(v.size()/(float)sub_div));
	ns_64_bit largest_delta(0);
	unsigned int i;
	for (i = 0; i < v.size(); i+=d_i){
		int s(i+d_i);
		if (i+d_i >= v.size())
			s = v.size()-1;
		if (v[s]-v[i] > largest_delta)
			largest_delta = v[s]-v[i];
	}
	return largest_delta;
}
unsigned long ns_time_path_image_movement_analyzer::calculate_division_size_that_fits_in_specified_memory_size(const ns_64_bit & mem_size, const int multiplicity_of_images)const{
//	std::vector<ns_64_bit> image_size(groups.size());
	bool largest_found(false);
	//generate the cumulative memory allocation needed to run the entire set;
	ns_image_properties largest_image_in_set(0,0,0,0);
	int count(0);
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path()){	
				continue;
			}
			count++;
			ns_image_properties p;
			groups[i].paths[j].set_path_alignment_image_dimensions(p);
			if (!largest_found){
				largest_image_in_set = p;
				largest_found = true;
			}
			else if (largest_image_in_set.width < p.width)
				largest_image_in_set.width  = p.width;
			else if (largest_image_in_set.height < p.height)
				largest_image_in_set.height  = p.height;
			//cumulative_size[i]=largest_image_in_set.width*(ns_64_bit)largest_image_in_set.height;
		}
	}
	
	const ns_64_bit total_mem_required((largest_image_in_set.width*(ns_64_bit)largest_image_in_set.height)*count);
	const ns_64_bit ret_i((total_mem_required*(ns_64_bit)multiplicity_of_images)/mem_size);
	if (ret_i*total_mem_required==mem_size)
		return ret_i;
	return ret_i+1;
}
void ns_time_path_image_movement_analyzer::calculate_memory_pool_maximum_image_size(const unsigned int start_group,const unsigned int stop_group){
	
	bool largest_found(false);
	ns_image_properties largest_image_in_set(0,0,0,0);
	for (unsigned int i = start_group; i < stop_group; i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
				continue;
			ns_image_properties p;
			groups[i].paths[j].set_path_alignment_image_dimensions(p);
			if (!largest_found){
				largest_image_in_set = p;
				largest_found = true;
			}
			else if (largest_image_in_set.width < p.width)
				largest_image_in_set.width  = p.width;
			else if (largest_image_in_set.height < p.height)
				largest_image_in_set.height  = p.height;
		}
	}
	memory_pool.set_overallocation_size(largest_image_in_set);
}


#ifdef NS_OUTPUT_ALGINMENT_DEBUG
string debug_path_name;
#endif
void ns_time_path_image_movement_analyzer::process_raw_images(const ns_64_bit region_id,const ns_time_path_solution & solution_, const ns_time_series_denoising_parameters & times_series_denoising_parameters,const ns_analyzed_image_time_path_death_time_estimator * e,ns_sql & sql, const long group_number,const bool write_status_to_db){
	analysis_id = ns_current_time();
	region_info_id = region_id;
	const unsigned long clear_lag((ns_analyzed_image_time_path::movement_time_kernel_width > ns_analyzed_image_time_path::alignment_time_kernel_width )?
									ns_analyzed_image_time_path::movement_time_kernel_width:ns_analyzed_image_time_path::alignment_time_kernel_width);
	try{

		externally_specified_plate_observation_interval = get_externally_specified_last_measurement(region_id,sql);
		load_from_solution(solution_);
		crop_path_observation_times(externally_specified_plate_observation_interval);

		acquire_region_image_specifications(region_id,sql);
		load_movement_image_db_info(region_id,sql);
		get_output_image_storage_locations(region_id,sql);

		unsigned long p(0);
	//	image_server.register_server_event(ns_image_server_event("Calculating Movement",false),&sql);
		

		//initiate chunk generator and alignment states
		const unsigned long chunk_size(10);
		
		//if we try to run too many worms simultaneously, we run out of memory on 32bit systems.
		//Most plates have fewer worms than this so usually the processing is done in one step.
		//But in situations with lots of worms we run through the region data files multiple times. 
		//This of course takes a lot longer.
		const unsigned long minimum_chunk_size(ns_analyzed_image_time_path::alignment_time_kernel_width);
		vector<vector<ns_chunk_generator> > chunk_generators;
		vector<vector<ns_alignment_state> > alignment_states;
		chunk_generators.resize(groups.size());
		alignment_states.resize(groups.size());
		
		long number_of_paths_to_consider(0),
				number_of_paths_to_ignore(0);
		for (unsigned int i = 0; i < groups.size(); i++){
			alignment_states[i].resize(groups[i].paths.size());
			chunk_generators[i].reserve(groups[i].paths.size());
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (!ns_skip_low_density_paths || !groups[i].paths[j].is_low_density_path())
					number_of_paths_to_consider++;
				else number_of_paths_to_ignore++;
				chunk_generators[i].push_back(ns_chunk_generator(chunk_size,groups[i].paths[j]));
			}
		}
		if (number_of_paths_to_consider == 0)
			image_server.register_server_event(ns_image_server_event("No dead animals, or potentially dead animals, were identified in this region."),&sql);
		else{
			if (write_status_to_db)
				image_server.register_server_event(ns_image_server_event("Registering and Analyzing ") << number_of_paths_to_consider << " objects; discarding " << number_of_paths_to_ignore << " as noise.",&sql);
			else
			std::cout << "Registering and Analyzing " << number_of_paths_to_consider << " objects; discarding " << number_of_paths_to_ignore << " as noise.\n";
			
			const bool system_is_64_bit(sizeof(void *)==8);
			const int number_of_images_stored_in_memory_per_group(
				3*chunk_size //the registered images
				+
				3*(2*chunk_size)
				+
				1 //intermediate buffers used during saves
				);

			ns_64_bit max_mem_per_node = (((ns_64_bit)image_server.maximum_memory_allocation_in_mb())*1024*1024)/
												image_server.number_of_node_processes_per_machine();
			ns_64_bit max_mem_on_32_bit = (((ns_64_bit)1)<<28)*7;  //1.75GB

			//32 bit systems become unreliable if you allocate > 1.75 GB.
			//Yes you can set flags to get access to 3GB but this is finniky in practice
			//and we err on the side of stability.
			if (!system_is_64_bit && max_mem_per_node > max_mem_on_32_bit)
				max_mem_on_32_bit = max_mem_on_32_bit;

			const int number_of_repeats_required(
				calculate_division_size_that_fits_in_specified_memory_size(
				max_mem_per_node, 
				number_of_images_stored_in_memory_per_group));

			if (number_of_repeats_required > number_of_paths_to_consider)
				throw ns_ex("The specified worms are so big that they cannot be processed in memory");
			//const int ((int)(ceil(number_of_paths_to_consider/(float)maximum_number_of_worms_to_process_simultaneously)));
			const int number_of_worms_per_repeat((int)(ceil(number_of_paths_to_consider/(float)number_of_repeats_required)));
			if (number_of_repeats_required > 1){
				if (write_status_to_db)
					image_server.register_server_event(ns_image_server_event( "To fit everything into memory, we're doing this in ") << number_of_repeats_required << " rounds",&sql);
				else
					std::cout << "To fit everything into memory, we're doing this in " << number_of_repeats_required << " rounds\n";
			}
			int current_round(0);
			for (unsigned int g = 0; g < groups.size(); ){
			
				const unsigned long start_group(g);
				unsigned long stop_group;
				{
					int number_to_do_this_round(0);
					bool found_enough(false);
					//find the last worm that 
					for (stop_group = start_group; stop_group < groups.size() &&!found_enough; stop_group++){
						for (unsigned int j = 0; j < groups[stop_group].paths.size(); j++){
							if (!ns_skip_low_density_paths || !groups[stop_group].paths[j].is_low_density_path()){
								if (number_to_do_this_round == number_of_worms_per_repeat){
									found_enough = true;
									stop_group--; //we've gone one too far
									break;
								}
								number_to_do_this_round++;	
							}
						}
					}
				}
				if (start_group == stop_group)
					break;
				//cerr << "Running group " << start_group << "," << stop_group << "\n";
				if (number_of_repeats_required > 1){
					if (write_status_to_db)
						image_server.register_server_event(ns_image_server_event("Starting Round ") <<  (current_round+1) << "\n",&sql);
					else
						std::cout << "\nStarting Round " <<  (current_round+1) << "\n";
				}
				calculate_memory_pool_maximum_image_size(start_group,stop_group);
				#ifdef NS_OUTPUT_ALGINMENT_DEBUG
				ofstream oout("c:\\tst_quant.csv");
				oout << "Frame,Offset X,Offset Y, Offset Magnitude\n";
				oout.flush();
				#endif
				
				unsigned long debug_output_skip(0);
				
				//first, we register backwards from onset of stationarity
				for (unsigned int i = start_group; i < stop_group; i++){
					for (unsigned int j = 0; j < groups[i].paths.size(); j++){
						groups[i].paths[j].find_first_labeled_stationary_timepoint();
					//	if (i == 24)
					//		cerr <<"SHA";
						chunk_generators[i][j].setup_first_chuck_for_backwards_registration();
					}
				}
				if (write_status_to_db)
						image_server.register_server_event(ns_image_server_event("Running backwards..."),&sql);
				else
				cerr << "Running Backwards...";
				for (long t = (long)region_image_specifications.size()-1;t>=0;t--){
					if(debug_output_skip==number_of_repeats_required || t == 0 && current_round == 0){
						std::cout << (100*(region_image_specifications.size()+1-t+region_image_specifications.size()*current_round))/(region_image_specifications.size()*number_of_repeats_required) << "%...";
						debug_output_skip = 0;
					}
					else debug_output_skip++;
					long stop_t(t-(long)chunk_size);
					if (stop_t < -1)
						stop_t = -1;
					try{	
						//only load images that are needed,
						//e.g those located between (t-chunk_size,t)
						for (long t1 = stop_t+1; t1 < t+1; t1++){

							bool image_needed(false);
							const unsigned long cur_time = region_image_specifications[t1].time;
							for (unsigned int i = start_group; i < stop_group && !image_needed ; i++){
								for (unsigned int j = 0; j < groups[i].paths.size(); j++){
									if (chunk_generators[i][j].first_chunk().start_i == -1)
										continue;
									if ( groups[i].paths[j].element(chunk_generators[i][j].first_chunk().start_i).absolute_time >= cur_time &&
										groups[i].paths[j].element(0).absolute_time  <= cur_time)
										image_needed = true;
									break;
								}
							}
							if (!image_needed){
					//			cerr  << "Skipping " << cur_time << "\n";
								continue;
							}
					//		cerr << "Loading " << cur_time << "\n";
							load_region_visualization_images(t1,t1+1,start_group,stop_group,sql,false);
						}
					}
					catch(ns_ex & ex){
						ns_ex ex_f(ex);
						image_server.register_server_event(ns_image_server::ns_register_in_central_db, ns_image_server_event("Found an error; doing a consistancy check on all images in the region: ") << ex.text());
						try{
							load_region_visualization_images(0,region_image_specifications.size(),0,groups.size(),sql,true);
						}
						catch(ns_ex & ex2){
							ex_f << ";" << ex2.text();
						}
						throw ex_f;
					}
					//deallocate everything we're not going to use
					for (unsigned int i = start_group; i < stop_group; i++){
						for (unsigned int j = 0; j < groups[i].paths.size(); j++){
					//		cerr << "Clearing " << i << ": " << chunk_generators[i][j].first_chunk().start_i+1 << "-" << groups[i].paths[j].elements.size() << "\n";
							for (long k = chunk_generators[i][j].first_chunk().start_i+1; k < groups[i].paths[j].elements.size(); k++){
									groups[i].paths[j].elements[k].clear_path_aligned_images();
									groups[i].paths[j].elements[k].clear_movement_images();
								}
						}
					}
					//run chunks for all paths whose images have been loaded in the previous step
					for (unsigned int i = start_group; i < stop_group; i++){
						for (unsigned int j = 0; j < groups[i].paths.size(); j++){
							if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
								continue;
							ns_analyzed_time_image_chunk chunk;
					
							if (!chunk_generators[i][j].backwards_update_and_check_for_new_chunk(chunk))
								continue;
				//			cerr << "Registering " << i << ": " << chunk.start_i << "-" << chunk.stop_i << "\n";
							groups[i].paths[j].calculate_image_registration(chunk,alignment_states[i][j],chunk_generators[i][j].first_chunk());
							groups[i].paths[j].generate_movement_images(chunk);
						
							if (chunk.stop_i == -1){
									ns_analyzed_time_image_chunk chunk;
									chunk.direction = ns_analyzed_time_image_chunk::ns_forward;
									chunk.start_i = 0;
									chunk.stop_i = groups[i].paths[j].first_stationary_timepoint();
									if (chunk.stop_i != 0){
										groups[i].paths[j].quantify_movement(chunk);
										groups[i].paths[j].save_movement_images(chunk,sql);
										for (long k = chunk.start_i; k < ((long)chunk.stop_i-(long)clear_lag); k++){
											groups[i].paths[j].elements[k].clear_path_aligned_images();
											groups[i].paths[j].elements[k].clear_movement_images();
										}
									}
									groups[i].paths[j].volatile_backwards_path_data_written = true;
									
								
							}
						}
					}

				}
				debug_output_skip = 0;
				//now we output (in forwards order) the images we registered backwards
				for (unsigned int i = start_group; i < stop_group; i++){
					for (unsigned int j = 0; j < groups[i].paths.size(); j++){
						if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
							continue;
						if (groups[i].paths[j].volatile_backwards_path_data_written)
							continue;
						ns_analyzed_time_image_chunk chunk;
						chunk.direction = ns_analyzed_time_image_chunk::ns_forward;
						chunk.start_i = 0;
						chunk.stop_i = groups[i].paths[j].first_stationary_timepoint();
						if (chunk.stop_i != 0){
							groups[i].paths[j].quantify_movement(chunk);
							groups[i].paths[j].save_movement_images(chunk,sql);
							for (long k = chunk.start_i; k < ((long)chunk.stop_i-(long)clear_lag); k++){
								groups[i].paths[j].elements[k].clear_path_aligned_images();
								groups[i].paths[j].elements[k].clear_movement_images();
							}
						}
					}
				}
				
				//now we go ahead and work forwards
				for (unsigned int i = start_group; i < stop_group; i++){
					for (unsigned int j = 0; j < groups[i].paths.size(); j++){
						chunk_generators[i][j].setup_first_chuck_for_forwards_registration();
					}
				}
				if (write_status_to_db)
						image_server.register_server_event(ns_image_server_event("Running forwards..."),&sql);
				std::cout << "\nRunning Forwards...";
				for (unsigned int t = 0; t < region_image_specifications.size(); t+=chunk_size){
					if(debug_output_skip==number_of_repeats_required || t == 0 && current_round == 0){
						std::cout << (100*(t+region_image_specifications.size()*current_round))/(region_image_specifications.size()*number_of_repeats_required) << "%...";
						debug_output_skip = 0;
					}
					else debug_output_skip++;
					//load a chunk of images
					unsigned long stop_t = t+chunk_size;
					if (stop_t > region_image_specifications.size())
						stop_t = region_image_specifications.size();
				//	cerr << "Loading images " << t << "-" << stop_t << "\n";
					try{
						load_region_visualization_images(t,stop_t,start_group,stop_group,sql,false);
					}
					catch(ns_ex & ex){
						ns_ex ex_f(ex);
						image_server.register_server_event(ns_image_server::ns_register_in_central_db, ns_image_server_event("Found an error; doing a consistancy check on all images in the region: ") << ex.text());
						try{
							load_region_visualization_images(0,region_image_specifications.size(),0,groups.size(),sql,true);
						}
						catch(ns_ex & ex2){
							ex_f << ";" << ex2.text();
						}
						throw ex_f;
					}
					for (unsigned int i = start_group; i < stop_group; i++){
						for (unsigned int j = 0; j < groups[i].paths.size(); j++){
							if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
								continue;
					
							#ifdef NS_OUTPUT_ALGINMENT_DEBUG
							if (i != 1)	//xxx
								continue;
							#endif
							
				//			if (i == 3)
				//				cerr << "WHA";
							ns_analyzed_time_image_chunk chunk;
							//only continue if a chunk's worth of data is loaded
							if (!chunk_generators[i][j].update_and_check_for_new_chunk(chunk))
								continue;
				//			cerr << "Processing path " << i <<"." << j << ":(" << chunk.start_i << "-" << chunk.stop_i << ")\n";
							#ifdef NS_OUTPUT_ALGINMENT_DEBUG
							cerr << "PATH " << i << "," << j << "\n";
							debug_path_name = string("path") + ns_to_string(i) + "_" + ns_to_string(j);
							#endif			
							groups[i].paths[j].calculate_image_registration(chunk,alignment_states[i][j],chunk_generators[i][j].first_chunk());
							//cerr << "Calculating Movement...";
							groups[i].paths[j].generate_movement_images(chunk);
							#ifdef NS_OUTPUT_ALGINMENT_DEBUG
								for (unsigned int k = chunk.start_i; k < chunk.stop_i; k++){
									oout << k << "," << groups[i].paths[j].element(k).registration_offset.x <<  "," <<
										groups[i].paths[j].element(k).registration_offset.y <<  "," <<
										groups[i].paths[j].element(k).registration_offset.mag() << "\n";
								}
								oout.flush();
							#endif
							groups[i].paths[j].quantify_movement(chunk);
							groups[i].paths[j].save_movement_images(chunk,sql);
						//	return;;
						//	cerr << "Clearing path aligned images up to " << (long)chunk.stop_i << ")\n";
							for (long k = 0; k < (long)chunk.stop_i; k++){
								groups[i].paths[j].elements[k].clear_movement_images();
							}
							//const unsigned long ss((chunk.start_i > clear_lag)?(chunk.start_i - clear_lag):0);
							for (long k = 0; k < ((long)chunk.stop_i-(long)clear_lag); k++){
								groups[i].paths[j].elements[k].clear_path_aligned_images();
							}
						}
					}
				
				}
				for (unsigned int i = start_group; i < stop_group; i++){
					for (unsigned int j = 0; j < groups[i].paths.size(); j++){
					//	if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
					//		continue;
						for (long k = 0; k < (long)groups[i].paths[j].elements.size(); k++){
							groups[i].paths[j].elements[k].clear_movement_images();
							groups[i].paths[j].elements[k].clear_path_aligned_images();
						}
					}
				}
				image_loading_temp.use_more_memory_to_avoid_reallocations(false);
				image_loading_temp.clear();
		
				for (unsigned int i = start_group; i < stop_group; i++){
					for (unsigned int j = 0; j < groups[i].paths.size(); j++){	
						if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
								continue;
						
						groups[i].paths[j].denoise_movement_series(times_series_denoising_parameters);
						//groups[i].paths[j].analyze_movement(e,ns_stationary_path_id(i,j,analysis_id));
						//groups[i].paths[j].calculate_movement_quantification_summary();
					}
				}
				memory_pool.clear();
				g=stop_group;
				current_round++;
			}
		}
		normalize_movement_scores_over_all_paths(times_series_denoising_parameters);
		for (unsigned int i = 0; i < groups.size(); i++){
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
						if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
								continue;
				groups[i].paths[j].analyze_movement(e,ns_stationary_path_id(i,j,analysis_id),last_timepoint_in_analysis_);
				groups[i].paths[j].calculate_movement_quantification_summary();
			}
		}

		//ofstream oo("c:\\out.csv");
		unsigned long total_groups(0);
		unsigned long total_skipped(0);
		for (unsigned int i = 0; i < groups.size(); i++){
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			//	for (unsigned int k = 0; k < groups[i].paths[j].elements.size(); k++)
			//		oo << i << "," << j << "," << k << "," << (groups[i].paths[j].elements[k].element_was_processed?1:0) << "\n";
				const unsigned long c(groups[i].paths[j].number_of_elements_not_processed_correctly());
				//if (c > 0){
				//}
				total_groups+=(c>0)?1:0;
				total_skipped+=c;
			}
		}
		//oo.close();
		if (total_skipped > 0){
			throw ns_ex("") << total_skipped << " frames were missed among " << total_groups << " out of a total of " << groups.size() << " path groups.";
		}
		generate_movement_description_series();
		mark_path_images_as_cached_in_db(region_id,sql);
		movement_analyzed = true;
	}
	catch(...){
		delete_from_db(region_id,sql);
		throw;
	}
	if (write_status_to_db)
			image_server.register_server_event(ns_image_server_event("Done."),&sql);
	else cerr << "Done.\n";
}

unsigned long ns_analyzed_image_time_path::number_of_elements_not_processed_correctly() const{
	unsigned long not_processed_correctly(0);
	for (unsigned int i = 0; i < elements.size(); i++){
		if (!elements[i].excluded && !elements[i].element_was_processed)
			not_processed_correctly++;
	}
	return not_processed_correctly;
}
	
/*void ns_analyzed_image_time_path::out_histograms(std::ostream & o) const{
	o << "Value,Stationary Count, Movement Count\n";
	for (unsigned long i = 0; i < 256; i++)
		o << i << "," << (unsigned long)stationary_histogram[i] << "," << (unsigned long)movement_histogram[i] << "\n";
}*/

void ns_time_path_image_movement_analyzer::load_from_solution(const ns_time_path_solution & solution_, const long group_number){
	solution = &solution_;
	groups.clear();
	extra_non_path_events.clear();

	if (region_info_id == 0)
		throw ns_ex("load_from_solution()::No Region ID Specified!");
	
	this->image_db_info_loaded = false;

	for (unsigned int i = 0; i < solution_.timepoints.size(); i++){
		if (solution_.timepoints[i].time > last_timepoint_in_analysis_)
			last_timepoint_in_analysis_ = solution_.timepoints[i].time;
	}
	number_of_timepoints_in_analysis_ = solution_.timepoints.size();
	groups.reserve(solution_.path_groups.size());
	if (group_number != -1){
		groups.push_back(ns_analyzed_image_time_path_group(group_number,region_info_id,solution_,externally_specified_plate_observation_interval,extra_non_path_events,memory_pool));
		for (unsigned int i = 0; i < groups.rbegin()->paths.size(); i++){
			if (groups.rbegin()->paths[i].elements.size() < ns_analyzed_image_time_path::alignment_time_kernel_width)
				throw ns_ex("ns_time_path_image_movement_analyzer::load_from_solution::Path loaded that is too short.");
		}
		if (groups.rbegin()->paths.size() == 0)
			groups.pop_back();
	}
	else{
		for (unsigned int i = 0; i < solution_.path_groups.size(); i++){
			
			groups.push_back(ns_analyzed_image_time_path_group(i,region_info_id,solution_,externally_specified_plate_observation_interval,extra_non_path_events,memory_pool));
			for (unsigned int j = 0; j < groups.rbegin()->paths.size(); j++){
				if (groups.rbegin()->paths[j].elements.size() < ns_analyzed_image_time_path::alignment_time_kernel_width)
					throw ns_ex("ns_time_path_image_movement_analyzer::load_from_solution::Path loaded that is too short.");
			}
			if (groups.rbegin()->paths.size() == 0)
				groups.pop_back();
		}
	}
	if (solution_.timepoints.size() == 0)
		return;

	//add annotations for fast moving animals
	unsigned long last_time = solution_.timepoints.rbegin()->time;
	const unsigned long current_time(ns_current_time());
	for (unsigned int i = 0; i < solution_.unassigned_points.stationary_elements.size(); i++){
		const ns_time_path_element &e(solution_.element(solution_.unassigned_points.stationary_elements[i]));
		std::string expl("NP");
		ns_death_time_annotation::ns_exclusion_type excluded(ns_death_time_annotation::ns_not_excluded);
		if (e.low_temporal_resolution)
			excluded = ns_death_time_annotation::ns_machine_excluded;  //animals marked as low temperal resolution
																	   //should be excluded and not used
																	   //in censoring calculations
		extra_non_path_events.add(
				ns_death_time_annotation(ns_fast_moving_worm_observed,
				0,region_info_id,
				ns_death_time_annotation_time_interval(solution_.time(solution_.unassigned_points.stationary_elements[i]),solution_.time(solution_.unassigned_points.stationary_elements[i])),
				e.region_position,
				e.region_size,
				excluded,
				ns_death_time_annotation_event_count(1+e.number_of_extra_worms_identified_at_location,0),
				current_time,ns_death_time_annotation::ns_lifespan_machine,
				(e.part_of_a_multiple_worm_disambiguation_cluster)?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
				ns_stationary_path_id(),false,e.inferred_animal_location,
				expl)
		);
	}

	
	get_processing_stats_from_solution(solution_);
}


void ns_time_path_image_movement_analyzer::get_output_image_storage_locations(const unsigned long region_id,ns_sql & sql){

	string region_name,sample_name,experiment_name;
	ns_64_bit sample_id,experiment_id;
	ns_region_info_lookup::get_region_info(region_id,&sql,region_name,sample_name,sample_id, experiment_name, experiment_id);
	
	ns_file_location_specification region_info(image_server.image_storage.get_base_path_for_region(region_id,&sql));
	sql << "SELECT id, image_id FROM path_data WHERE region_id = " << region_id << " AND group_id >= " << groups.size();
	ns_sql_result res;
	sql.get_rows(res);
	for (unsigned int i = 0; i < res.size(); i++){
		ns_image_server_image im;
		im.load_from_db(atol(res[i][1].c_str()),&sql);
		image_server.image_storage.delete_from_storage(im,ns_delete_both_volatile_and_long_term,&sql);
		sql << "DELETE FROM images WHERE id = " << im.id;
		sql.send_query();
	}
	if (res.size() > 0){
		sql << "DELETE FROM path_data WHERE region_id = " << region_id << " AND group_id >= " << groups.size();
		sql.send_query();
	}

	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
				continue;
			ns_image_server_image & im(groups[i].paths[j].output_image);
			if (im.id == 0){
				im = image_server.image_storage.get_storage_for_path(region_info, j, i,
																region_id, region_name,experiment_name, sample_name);
				im.save_to_db(0,&sql);
				if (im.id == 0)
					throw ns_ex("ns_time_path_image_movement_analyzer::save_movement_images()::Could not generate a new path id image id.");
				if (groups[i].paths[j].path_db_id == 0){
					sql << "INSERT INTO path_data SET image_id = " << im.id << ", region_id = " << region_id << ",group_id = " << i << ",path_id = " << j;
					groups[i].paths[j].path_db_id = sql.send_query_get_id();
				}
				else{
					sql << "UPDATE path_data SET image_id = " << im.id << " WHERE id = " << groups[i].paths[j].path_db_id;
					sql.send_query();
				}
			}
		}
	}
}
ns_death_time_annotation_time_interval ns_time_path_image_movement_analyzer::get_externally_specified_last_measurement(const unsigned long region_id, ns_sql & sql){
	sql << "SELECT time_of_last_valid_sample FROM sample_region_image_info WHERE id = " << region_id;
	ns_sql_result res;
	sql.get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_time_path_image_movement_analyzer::crop_paths_to_final_time()::Could not load time of last sample from database.");
	unsigned long stop_time(atol(res[0][0].c_str()));
	if (stop_time == 0)
		stop_time = UINT_MAX;
	return ns_death_time_annotation_time_interval(0,stop_time);
}
void ns_time_path_image_movement_analyzer::crop_path_observation_times(const ns_death_time_annotation_time_interval & val){
	
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			if(groups[i].paths[j].elements.size() == 0)
				continue;

			for (unsigned int k = 0; k < groups[i].paths[j].elements.size(); k++){
				if (groups[i].paths[j].elements[k].absolute_time > val.period_end ||
					groups[i].paths[j].elements[j].absolute_time < val.period_start)
					groups[i].paths[j].elements[k].excluded = true;
			}
		/*	std::vector<ns_analyzed_image_time_path_element>::iterator p = groups[i].paths[j].elements.end();
			p--;
			while(true){
				bool is_first_element(p == groups[i].paths[j].elements.begin());
				if (p->absolute_time > last_time){
					p = groups[i].paths[j].elements.erase(p);
				}
				if ( groups[i].paths[j].elements.empty() || is_first_element)
					break;
				p--;
			}
		}*/
		}
	}
	for (std::vector<ns_death_time_annotation>::iterator p = extra_non_path_events.events.begin(); p != extra_non_path_events.events.end();){
		if (p->time.period_end > val.period_end ||
			p->time.period_start < val.period_start)
			p = extra_non_path_events.erase(p);
		else ++p;
	}
}

void ns_time_path_image_movement_analyzer::populate_movement_quantification_from_file(ns_sql & sql){
	
	sql << "SELECT movement_image_analysis_quantification_id FROM sample_region_image_info WHERE id = " << this->region_info_id;
	ns_sql_result res;
	sql.get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_time_path_image_movement_analyzer::load_movement_data_from_db():Could not load info from db");
	ns_image_server_image im;
	im.id = atol(res[0][0].c_str());
	if (im.id == 0)
		throw ns_ex("Movement quantification data has not been stored in db");
	ifstream * i(image_server.image_storage.request_metadata_from_disk(im,false,&sql));

	try{
		load_movement_data_from_disk(*i,false);
		delete i;
	}
	catch(...){
		delete i;
		throw;
	}
};

void ns_time_path_image_movement_analyzer::reanalyze_with_different_movement_estimator(const ns_time_series_denoising_parameters &,const ns_analyzed_image_time_path_death_time_estimator * e){
	if (region_info_id == 0)
		throw ns_ex("Attempting to reanalyze an unloaded image!");
	
	for (unsigned long g = 0; g < groups.size(); g++)
		for (unsigned long p = 0; p < groups[g].paths.size(); p++){
			if (ns_skip_low_density_paths && groups[g].paths[p].is_low_density_path())
				continue;
			groups[g].paths[p].analyze_movement(e,ns_stationary_path_id(g,p,analysis_id),last_timepoint_in_analysis_);
			groups[g].paths[p].calculate_movement_quantification_summary();
		}
}
bool ns_time_path_image_movement_analyzer::load_completed_analysis(const ns_64_bit region_id,const ns_time_path_solution & solution_,  const ns_time_series_denoising_parameters & times_series_denoising_parameters, const ns_analyzed_image_time_path_death_time_estimator * e,ns_sql & sql, bool exclude_movement_quantification){
	region_info_id = region_id;
	externally_specified_plate_observation_interval = get_externally_specified_last_measurement(region_id,sql);
	load_from_solution(solution_);
	crop_path_observation_times(externally_specified_plate_observation_interval);
	bool found_path_info_in_db = load_movement_image_db_info(region_info_id,sql);


	
	for (unsigned long g = 0; g < groups.size(); g++){
		for (unsigned long p = 0; p < groups[g].paths.size(); p++)
			for (unsigned int i = 0; i < groups[g].paths[p].death_time_annotations().events.size(); i++){
			//	groups[g].paths[p].by_hand_annotation_event_times.resize((int)ns_number_of_movement_event_types,-1);
				groups[g].paths[p].death_time_annotation_set.events[i].region_info_id = region_info_id;
			}
	}

	populate_movement_quantification_from_file(sql);
	if (exclude_movement_quantification)
		return found_path_info_in_db;
	
	for (unsigned long g = 0; g < groups.size(); g++)
		for (unsigned long p = 0; p < groups[g].paths.size(); p++){
			unsigned long number_of_valid_points(0);
			for (unsigned int i = 0; i < groups[g].paths[p].element_count(); i++){
				if (!groups[g].paths[p].element(i).censored && 
					!groups[g].paths[p].element(i).excluded)
					number_of_valid_points++;
			}
			if (ns_skip_low_density_paths && groups[g].paths[p].is_low_density_path())
				continue;
			//invalid paths can result from changing the last valid timepoint and reanalyzing saved movement analyses
			if (number_of_valid_points <= 1 || groups[g].paths[p].element_count() > 0 && groups[g].paths[p].element(0).absolute_time > this->last_timepoint_in_analysis()){
				groups[g].paths[p].entirely_excluded = true;
				continue;
			}
		//	if (g == 5)
		//		cerr << "WHA";
			groups[g].paths[p].denoise_movement_series(times_series_denoising_parameters);
		}

	normalize_movement_scores_over_all_paths(times_series_denoising_parameters);

	for (unsigned long g = 0; g < groups.size(); g++)
		for (unsigned long p = 0; p < groups[g].paths.size(); p++){
			unsigned long number_of_valid_points(0);
			for (unsigned int i = 0; i < groups[g].paths[p].element_count(); i++){
				if (!groups[g].paths[p].element(i).censored && 
					!groups[g].paths[p].element(i).excluded)
					number_of_valid_points++;
			}
			if (ns_skip_low_density_paths && groups[g].paths[p].is_low_density_path())
				continue;
			//invalid paths can result from changing the last valid timepoint and reanalyzing saved movement analyses
			if (number_of_valid_points <= 1 || groups[g].paths[p].element_count() > 0 && groups[g].paths[p].element(0).absolute_time > this->last_timepoint_in_analysis()){
				groups[g].paths[p].entirely_excluded = true;
				continue;
			}
		//	if (g == 5)
		//		cerr << "WHA";
			groups[g].paths[p].analyze_movement(e,ns_stationary_path_id(g,p,analysis_id),last_timepoint_in_analysis_);
			groups[g].paths[p].calculate_movement_quantification_summary();
		}	


	//generate_movement_description_series();
	movement_analyzed = true;
	return found_path_info_in_db;
}
void ns_time_path_image_movement_analyzer::save_movement_data_to_db(const unsigned long region_id, ns_sql & sql){

	sql << "SELECT movement_image_analysis_quantification_id FROM sample_region_image_info WHERE id = " << region_id;
	ns_sql_result res;
	sql.get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_time_path_image_movement_analyzer::save_movement_data_to_db():Could not load info from db");
	ns_image_server_image im;
	im.id = atol(res[0][0].c_str());
	bool update_db(false);
	if (im.id == 0){
		im = image_server.image_storage.get_region_movement_metadata_info(region_id,"time_path_movement_image_analysis_quantification",sql);
		update_db = true;
	}
	ofstream * o(image_server.image_storage.request_metadata_output(im,"csv",false,&sql));
	im.save_to_db(im.id,&sql);
	try{
		save_movement_data_to_disk(*o);
		delete o;
	}
	catch(...){
		delete o;
		throw;
	}
	if (update_db){
		sql << "UPDATE sample_region_image_info SET movement_image_analysis_quantification_id = " << im.id << " WHERE id = " << region_id;
		sql.send_query();
	}

}



void ns_time_path_image_movement_analyzer::load_movement_data_from_disk(istream & in, bool skip_movement_data){

	ns_get_int get_int;
	ns_get_double get_double;
	get_int(in,this->analysis_id);
	if (in.fail())
		throw ns_ex("Empty Specification!");
	if (skip_movement_data)
		return;
		
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			for (unsigned int k = 0; k < groups[i].paths[j].elements.size(); k++){
				groups[i].paths[j].elements[k].absolute_time = 0;
			}
		}
	}

	while(true){
		unsigned long group_id,path_id,element_id;
		get_int(in,group_id);
		if(in.fail()) break;

		get_int(in,path_id);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,element_id);
		if(in.fail()) throw ns_ex("Invalid Specification");

		if (group_id >= groups.size())
			throw ns_ex("ns_time_path_image_movement_analyzer::load_movement_data_from_disk()::Invalid group id ") << group_id;
		if (path_id >= groups[group_id].paths.size())
			throw ns_ex("ns_time_path_image_movement_analyzer::load_movement_data_from_disk()::Invalid path id ") << path_id;
		if (groups[group_id].paths[path_id].elements.size() == 0)
			throw ns_ex("ns_time_path_image_movement_analyzer::load_movement_data_from_disk()::Encountered an empty path");
		if (element_id >= groups[group_id].paths[path_id].elements.size())
			throw ns_ex("ns_time_path_image_movement_analyzer::load_movement_data_from_disk()::Element is too large ") << path_id;
		get_int(in,groups[group_id].paths[path_id].elements[element_id].absolute_time);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.interframe_time_scaled_movement_sum);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.movement_alternate_worm_sum);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_double(in,groups[group_id].paths[path_id].elements[element_id].measurements.change_in_average_region_intensity);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_double(in,groups[group_id].paths[path_id].elements[element_id].measurements.change_in_average_normalized_worm_intensity);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.total_worm_area);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.total_intensity_within_worm_area);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.total_region_area);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.total_intensity_within_region);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.total_alternate_worm_area);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.total_intensity_within_alternate_worm);
		if(in.fail()) throw ns_ex("Invalid Specification");

		int t;
		get_int(in,t);
		if(in.fail()) throw ns_ex("Invalid Specification");
		groups[group_id].paths[path_id].elements[element_id].saturated_offset = (t!=0);

		get_double(in,groups[group_id].paths[path_id].elements[element_id].registration_offset.x);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_double(in,groups[group_id].paths[path_id].elements[element_id].registration_offset.y);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_movement_sum);
		if(in.fail()) throw ns_ex("Invalid Specification");
		char a(get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_stationary_sum));
		if(in.fail()) throw ns_ex("Invalid Specification");
		if (a == '\n'){
			//old style records
			groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_position = ns_vector_2i(0,0);
			groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_area = ns_vector_2i(0,0);
			continue;
		}
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_position.x);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_position.y);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_area.x);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.local_maximum_area.y);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.unnormalized_movement_sum);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_double(in,groups[group_id].paths[path_id].elements[element_id].measurements.denoised_movement_score);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_double(in,groups[group_id].paths[path_id].elements[element_id].measurements.movement_score);
		if(in.fail()) throw ns_ex("Invalid Specification");
		get_int(in,groups[group_id].paths[path_id].elements[element_id].measurements.change_in_total_worm_intensity);
		if(in.fail()) throw ns_ex("Invalid Specification");

		string tmp;
		//open for future use
		for (unsigned int i = 0; i < 1; i++){
			get_int(in,tmp);
				if(in.fail()) throw ns_ex("Invalid Specification");
		}

	}
	//check all data is loaded
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			for (unsigned int k = 0; k < groups[i].paths[j].elements.size(); k++){
				if (groups[i].paths[j].elements[k].absolute_time == 0)
					throw ns_ex(" ns_time_path_image_movement_analyzer::load_movement_data_from_disk():Not all data specified in file!");
			}
		}
	}
}
void ns_time_path_image_movement_analyzer::get_processing_stats_from_solution(const ns_time_path_solution & solution_){
	number_of_timepoints_in_analysis_ = solution_.timepoints.size();
	
	last_timepoint_in_analysis_ = 0;
	for (unsigned int i = 0; i < solution_.timepoints.size(); i++){
		if (solution_.timepoints[i].time > last_timepoint_in_analysis_)
			last_timepoint_in_analysis_ = solution_.timepoints[i].time;
	}

}

void ns_time_path_image_movement_analyzer::save_movement_data_to_disk(ostream & o) const{
	o << this->analysis_id << "\n";
	for (unsigned long i = 0; i < groups.size(); i++){
		for (unsigned long j = 0; j < groups[i].paths.size(); j++){
			for (unsigned long k = 0; k < groups[i].paths[j].elements.size(); k++){
				o << i << ","
				  << j << "," 
				  << k << ","
				  << groups[i].paths[j].elements[k].absolute_time << ","
				  << groups[i].paths[j].elements[k].measurements.interframe_time_scaled_movement_sum << ","
				  << groups[i].paths[j].elements[k].measurements.movement_alternate_worm_sum << ","
				  << groups[i].paths[j].elements[k].measurements.change_in_average_region_intensity << ","
				  << groups[i].paths[j].elements[k].measurements.change_in_average_normalized_worm_intensity << ","
				  << groups[i].paths[j].elements[k].measurements.total_worm_area << ","
				  << groups[i].paths[j].elements[k].measurements.total_intensity_within_worm_area << ","
				  << groups[i].paths[j].elements[k].measurements.total_region_area << ","
				  << groups[i].paths[j].elements[k].measurements.total_intensity_within_region << ","
				  << groups[i].paths[j].elements[k].measurements.total_alternate_worm_area << ","
				  << groups[i].paths[j].elements[k].measurements.total_intensity_within_alternate_worm << ","
				  << (groups[i].paths[j].elements[k].saturated_offset?"1":"0") << ","
				  << groups[i].paths[j].elements[k].registration_offset.x << ","
				  << groups[i].paths[j].elements[k].registration_offset.y <<","
				  << groups[i].paths[j].elements[k].measurements.local_maximum_movement_sum << ","
				  << groups[i].paths[j].elements[k].measurements.local_maximum_stationary_sum << ","
				  << groups[i].paths[j].elements[k].measurements.local_maximum_position.x << ","
				  << groups[i].paths[j].elements[k].measurements.local_maximum_position.y << ","
				  << groups[i].paths[j].elements[k].measurements.local_maximum_area.x << ","
				  << groups[i].paths[j].elements[k].measurements.local_maximum_area.y << ","
				  << groups[i].paths[j].elements[k].measurements.unnormalized_movement_sum<<","
				  << groups[i].paths[j].elements[k].measurements.denoised_movement_score << ","
				  << groups[i].paths[j].elements[k].measurements.movement_score << ","
				  << groups[i].paths[j].elements[k].measurements.change_in_total_worm_intensity << ","
				  <<"\n";
			}
		}
	}
}


std::string ns_calc_rel_time_by_index(const unsigned long time, const ns_movement_state_observation_boundary_interval & i, const ns_analyzed_image_time_path & path){
	if (i.skipped)
		return "";
	return ns_to_string_short((((long)time)-(long)path.state_entrance_interval_time(i).best_estimate_event_time_for_possible_partially_unbounded_interval())/(60.0*60*24),3);
}


std::string ns_normalize_indexed_time(const ns_movement_state_observation_boundary_interval & i, const unsigned long time, const ns_analyzed_image_time_path & path){
	if (i.skipped)
		return "";
	return ns_to_string_short((((long)(long)path.state_entrance_interval_time(i).best_estimate_event_time_for_possible_partially_unbounded_interval()-(long)time))/(60.0*60*24),4);
}


std::string ns_normalize_abs_time(const long abs_time, const unsigned long time){
	if (abs_time == -1)
		return "";
	return ns_to_string_short((((long)abs_time-(long)time))/(60.0*60*24),4);
}

std::string ns_output_interval_difference(const unsigned long time, const ns_death_time_annotation_time_interval & b){
	if (b.period_end_was_not_observed)
		return "";
	return ns_to_string_short(((double)time-(double)b.period_end)/(60.0*60*24),3);
}


void ns_analyzed_image_time_path::write_summary_movement_quantification_analysis_header(std::ostream & o){
	ns_region_metadata::out_JMP_plate_identity_header(o);
	o << ",";
	o << "By Handed Excluded, By Hand Censored, Misc Flags,"
	  << "Group ID, Path ID, Absolute Position X, Absolute Position Y,"
		 "Machine Death Time, Machine Slow Movement Cessation Time, Machine Fast Movement Cessation Time,"
		 "By Hand Death Time, By Hand Slow Movement Cessation Time, By Hand Fast Movement Cessation Time,"
		 "Worm Intensity Mean Overall, Worm Intensity Variability Overall, Neighborhood Intensity Mean Overall, Neighborhood Intensity Variability Overall, Alternate Worm Intensity Mean Overall, Alternate Worm Intensity Variability Overall,"
		 "Worm Intensity Mean Before Death, Worm Intensity Variability Before Death, Neighborhood Intensity Mean Before Death, Neighborhood Intensity Variability Before Death, Alternate Worm Intensity Mean Before Death, Alternate Worm Intensity Variability Before Death,"
		
		 "Worm Registration Offset X Mean Overall, Worm Registration Offset X Variability Overall,"
		 "Worm Registration Offset Y Mean Overall, Worm Registration Offset Y Variability Overall,"
		 "Worm Registration Offset Magnitude Mean Overall, Worm Registration Offset Magnitude Variability Overall,"
		 "Worm Area Mean Overall, Worm Area Variability Overall, Registration Region Area Mean Overall, Registration Region Area Variability Overall, Worm Area to Region Area Average Ratio Overall,"
		
		 "Worm Registration Offset X Mean Before Death, Worm Registration Offset X Variability Before Death,"
		 "Worm Registration Offset Y Mean Before Death, Worm Registration Offset Y Variability Before Death,"
		 "Worm Registration Offset Magnitude Mean Before Death, Worm Registration Offset Magnitude Variability Before Death,"
		 "Worm Area Mean Before Death, Worm Area Variability Before Death, Registration Region Area Mean Before Death, Registration Region Area Variability Before Death, Worm Area to Region Area Average Ratio Before Death,"
		
		 "Number of Frames Before Death, Number of Frames After Death, Death as % of frames,"
		 "Average Movement Ratio Before Death, % Saturated Frames Before Death, % Saturated Frames Total";
}


void ns_analyzed_image_time_path::write_analysis_optimization_data_header(std::ostream & o){
	o << "Experiment,Device,Plate Name,Animal Details,Group ID,Path ID,Excluded,Censored,Number of Worms in Clump,"
		"Movement Threshold, Min Hold Time (Hours), Denoising Technique Used, "
		"Visual Inspection Death Age (Days),Machine Death Age (Days), Visual Inspection Death Time (Date), Difference Between Machine and By Hand Death Times (Days), Difference Squared, Random Group";
}

std::vector< std::vector < unsigned long > > static_messy_death_time_matrix;
void ns_analyzed_image_time_path::write_analysis_optimization_data(const ns_stationary_path_id & id, const std::vector<double> & thresholds, const std::vector<double> & hold_times, const ns_region_metadata & m,const ns_time_series_denoising_parameters & denoising_parameters,std::ostream & o) const{
	
	long death_time(by_hand_annotation_event_times[(int)ns_movement_cessation].period_end);
	if (by_hand_annotation_event_times[(int)ns_movement_cessation].fully_unbounded())
		return;
	calculate_analysis_optimization_data(thresholds,hold_times,static_messy_death_time_matrix);
	const unsigned long random_group(rand()%2);
	for (unsigned int i = 0; i < thresholds.size(); i++)
		for (unsigned int j = 0; j < hold_times.size(); j++){
			const double err(((double)static_messy_death_time_matrix[i][j] - death_time)/(60.0*60.0*24));
			o << m.experiment_name << "," << m.device << "," << m.plate_name() << "," << m.plate_type_summary() 
				<< "," << id.group_id << "," << id.path_id << ","
				<< (censoring_and_flag_details.is_excluded()?"1":"0") << ","
				<< (censoring_and_flag_details.is_censored()?"1":"0") << ","
				<< censoring_and_flag_details.number_of_worms() << ","
				<< thresholds[i] << "," << (hold_times[j])/60.0/60.0 << "," 
				<< denoising_parameters.to_string() << ","
				<< (death_time - m.time_at_which_animals_had_zero_age)/(60.0*60.0*24)  << ","
				<< (static_messy_death_time_matrix[i][j] - m.time_at_which_animals_had_zero_age)/(60.0*60.0*24)  << ","
				<< death_time << ","
				<< err << "," << sqrt(err*err) << "," << random_group << "\n";
		}
}

void ns_analyzed_image_time_path::calculate_analysis_optimization_data(const std::vector<double> & thresholds, const std::vector<double> & hold_times, std::vector< std::vector < unsigned long > > & death_times) const{
	//find first valid observation
	unsigned long start_i(0);
	for (start_i = 0; start_i < elements.size(); start_i++){
		if (!elements[start_i].excluded && !elements[start_i].element_before_fast_movement_cessation)
			break;
	}
	if (start_i == elements.size())
		throw ns_ex("No valid measurements found in the time path.");
	std::vector<std::vector<unsigned long> > last_time_point_at_which_movement_was_found(
	thresholds.size(),
	std::vector<unsigned long>(hold_times.size(),elements[start_i].absolute_time));

	death_times.resize(0);
	death_times.resize(thresholds.size(),std::vector<unsigned long>(hold_times.size(),0));

	for (long t = start_i; t < elements.size(); t++){
		
		if (elements[t].excluded) continue;

		double r(elements[t].measurements.death_time_posture_analysis_measure());
		const unsigned long &cur_time (elements[t].absolute_time);
		//keep on pushing the last posture time and last sationary
		//times forward until we hit a low enough movement ratio
		//to meet the criteria.  At that point, the last posture 
		//and last stationary cutoffs stick
		
		for (unsigned int thresh = 0; thresh < thresholds.size(); thresh++){
			for (unsigned int hold_t = 0; hold_t < hold_times.size(); hold_t++){
				if (death_times[thresh][hold_t] != 0) continue;
				if (r >= thresholds[thresh])
					last_time_point_at_which_movement_was_found[thresh][hold_t] = cur_time;
				if (death_times[thresh][hold_t] == 0){
					unsigned long dt;
					if (last_time_point_at_which_movement_was_found[thresh][hold_t] == 0)
						dt = cur_time - elements[start_i].absolute_time;
					else dt = cur_time - last_time_point_at_which_movement_was_found[thresh][hold_t];
					if (dt >= hold_times[hold_t])
						death_times[thresh][hold_t] = last_time_point_at_which_movement_was_found[thresh][hold_t];
				}
			}
		}
	}
	for (unsigned int thresh = 0; thresh < thresholds.size(); thresh++){
			for (unsigned int hold_t = 0; hold_t < hold_times.size(); hold_t++){
				if (death_times[thresh][hold_t] == 0)
					death_times[thresh][hold_t] = elements.rbegin()->absolute_time;
			}
	}
}


void ns_analyzed_image_time_path::write_summary_movement_quantification_analysis_data(const ns_region_metadata & m, const unsigned long group_id, const unsigned long path_id, std::ostream & o)const{
	m.out_JMP_plate_identity_data(o);
	o << ",";
	o << (censoring_and_flag_details.is_excluded()?ns_death_time_annotation::exclusion_value(censoring_and_flag_details.excluded):0) << ","
		<< (censoring_and_flag_details.is_censored()?ns_death_time_annotation::exclusion_value(censoring_and_flag_details.excluded):0) << ","
		<< censoring_and_flag_details.flag.label() << ",";
	o << group_id<<","<<path_id<<","
		<< path_region_position.x << ","
		<< path_region_position.y << ","
		<< ns_normalize_indexed_time(state_intervals[(int)ns_movement_stationary],m.time_at_which_animals_had_zero_age,*this) << ","
		<< ns_normalize_indexed_time(state_intervals[(int)ns_movement_posture],m.time_at_which_animals_had_zero_age,*this) << ","
		<< ns_normalize_indexed_time(state_intervals[(int)ns_movement_slow],m.time_at_which_animals_had_zero_age,*this) << ","
		<< ns_normalize_abs_time(by_hand_annotation_event_times[(int)ns_movement_cessation].period_end,m.time_at_which_animals_had_zero_age) << ","
		<< ns_normalize_abs_time(by_hand_annotation_event_times[(int)ns_translation_cessation].period_end,m.time_at_which_animals_had_zero_age) << ","
		<< ns_normalize_abs_time(by_hand_annotation_event_times[(int)ns_fast_movement_cessation].period_end,m.time_at_which_animals_had_zero_age) << ",";
	o << quantification_summary.mean_all.mean_intensity_within_worm_area << "," << quantification_summary.variability_all.mean_intensity_within_worm_area << ","
	  << quantification_summary.mean_all.mean_intensity_within_region << "," << quantification_summary.variability_all.mean_intensity_within_region << ","
	  << quantification_summary.mean_all.mean_intensity_within_alternate_worm << "," << quantification_summary.variability_all.mean_intensity_within_alternate_worm << ",";
	
	o << quantification_summary.mean_before_death.mean_intensity_within_worm_area << "," << quantification_summary.variability_before_death.mean_intensity_within_worm_area << ","
	  << quantification_summary.mean_before_death.mean_intensity_within_region << "," << quantification_summary.variability_before_death.mean_intensity_within_region << ","
	  << quantification_summary.mean_before_death.mean_intensity_within_alternate_worm << "," << quantification_summary.variability_before_death.mean_intensity_within_alternate_worm << ",";
	
	o << quantification_summary.mean_all.registration_displacement.x << "," << quantification_summary.variability_all.registration_displacement.x << ","
		<< quantification_summary.mean_all.registration_displacement.y << "," << quantification_summary.variability_all.registration_displacement.y << ","
		<< quantification_summary.mean_all.registration_displacement.mag() << "," << quantification_summary.variability_all.registration_displacement.mag() << ","

	  << quantification_summary.mean_all.total_worm_area << "," << quantification_summary.variability_all.total_worm_area << ","
	  << quantification_summary.mean_all.total_region_area << "," << quantification_summary.variability_all.total_region_area << ",";
	if (quantification_summary.mean_all.total_region_area != 0)
		o << quantification_summary.mean_all.total_worm_area/(double)quantification_summary.mean_all.total_region_area << ",";
	else o<< "0,";

	o << quantification_summary.mean_before_death.registration_displacement.x << "," << quantification_summary.variability_before_death.registration_displacement.x << ","
	  << quantification_summary.mean_before_death.registration_displacement.y << "," << quantification_summary.variability_before_death.registration_displacement.y << ","
	  << quantification_summary.mean_before_death.registration_displacement.mag() << "," << quantification_summary.variability_before_death.registration_displacement.mag() << ","
	  << quantification_summary.mean_before_death.total_worm_area << "," << quantification_summary.variability_before_death.total_worm_area << ","
	  << quantification_summary.mean_before_death.total_region_area << "," << quantification_summary.variability_before_death.total_region_area << ",";
	if (quantification_summary.mean_all.total_region_area != 0)
		o << quantification_summary.mean_all.total_worm_area/(double)quantification_summary.mean_all.total_region_area << ",";
	else o << "0,";
	o << quantification_summary.count_before_death << "," << quantification_summary.count_after_death << 100*quantification_summary.count_before_death/(double)quantification_summary.count_all << ","
		<< quantification_summary.mean_before_death.new_movement_ratio() << ",";
	if (quantification_summary.count_before_death != 0)
		o << 100*quantification_summary.number_of_registration_saturated_frames_before_death/(double)quantification_summary.count_before_death << ",";
	else o << "0,";
	if (quantification_summary.count_after_death != 0)
		o << 100*quantification_summary.number_of_registration_saturated_frames_after_death/(double)quantification_summary.count_after_death << ",";
	else o << "0,";
	if (quantification_summary.count_all != 0)
		o << 100*(quantification_summary.number_of_registration_saturated_frames_before_death+quantification_summary.number_of_registration_saturated_frames_after_death)/(double)quantification_summary.count_all << "\n";
	else o << "0\n";

}
void ns_analyzed_image_time_path::calculate_movement_quantification_summary(){

	quantification_summary.mean_before_death.zero();
	quantification_summary.mean_after_death.zero();
	quantification_summary.variability_all.zero();
	quantification_summary.variability_before_death.zero();
	quantification_summary.variability_after_death.zero();
	quantification_summary.mean_all.zero();
	quantification_summary.count_after_death = 0;
	quantification_summary.count_before_death = 0;
	quantification_summary.count_all = 0;
	quantification_summary.number_of_registration_saturated_frames_before_death = 0;
	quantification_summary.number_of_registration_saturated_frames_after_death = 0;
	for (unsigned int i  = 0; i < elements.size(); i++)
		elements[i].measurements.registration_displacement = elements[i].registration_offset;

	const long death_index((!state_intervals[(int)ns_movement_stationary].skipped)?
		state_intervals[(int)ns_movement_stationary].entrance_interval.period_end_index:(-1));
	if (death_index != -1){
		for (unsigned int i = 0; i <= death_index; i++){
			quantification_summary.count_before_death++;
			elements[i].measurements.calculate_means();
			quantification_summary.mean_before_death = quantification_summary.mean_before_death + elements[i].measurements;
			if (elements[i].saturated_offset)
				quantification_summary.number_of_registration_saturated_frames_before_death++;
		}
	}	
	for (unsigned int i = death_index+1; i < elements.size();i++){
	quantification_summary.count_after_death++;
		elements[i].measurements.calculate_means();
		quantification_summary.mean_after_death = quantification_summary.mean_after_death + elements[i].measurements;
		if (elements[i].saturated_offset)
			quantification_summary.number_of_registration_saturated_frames_after_death++;
	}
	
	quantification_summary.count_all = quantification_summary.count_before_death + quantification_summary.count_after_death;
	quantification_summary.mean_all = quantification_summary.mean_before_death + quantification_summary.mean_after_death;
	
	if (quantification_summary.count_all != 0) 
		quantification_summary.mean_all = quantification_summary.mean_all / quantification_summary.count_all;
	if (quantification_summary.count_before_death != 0) 
		quantification_summary.mean_before_death = quantification_summary.mean_before_death / quantification_summary.count_before_death;
	if (quantification_summary.count_after_death != 0) 
		quantification_summary.mean_after_death = quantification_summary.mean_after_death / quantification_summary.count_after_death;

	if (death_index != -1){
		for (unsigned int i = 0; i <= death_index; i++){
			
			ns_analyzed_image_time_path_element_measurements dif((elements[i].measurements + quantification_summary.mean_before_death/-1));
			dif.square();
			quantification_summary.variability_before_death = quantification_summary.variability_before_death + dif;

			dif = (elements[i].measurements + quantification_summary.mean_all/-1);
			dif.square();
			quantification_summary.variability_all = quantification_summary.variability_all + dif;
		}
	}	
	for (unsigned int i = death_index+1; i < elements.size();i++){
		ns_analyzed_image_time_path_element_measurements dif((elements[i].measurements + quantification_summary.mean_after_death/-1));
		dif.square();
		quantification_summary.variability_after_death = quantification_summary.variability_after_death + dif;

		dif = (elements[i].measurements + quantification_summary.mean_all/-1);
		dif.square();
		quantification_summary.variability_all = quantification_summary.variability_all + dif;
	}

	quantification_summary.variability_after_death.square_root();
	quantification_summary.variability_before_death.square_root();
	quantification_summary.variability_all.square_root();

	if (quantification_summary.count_all != 0) 
		quantification_summary.variability_all = quantification_summary.variability_all / quantification_summary.count_all;
	if (quantification_summary.count_before_death != 0) 
		quantification_summary.variability_before_death = quantification_summary.variability_before_death / quantification_summary.count_before_death;
	if (quantification_summary.count_after_death != 0) 
		quantification_summary.variability_after_death = quantification_summary.variability_after_death / quantification_summary.count_after_death;
}
void ns_analyzed_image_time_path::write_detailed_movement_quantification_analysis_header(std::ostream & o){
	ns_region_metadata::out_JMP_plate_identity_header_short(o);
	o << ",";
	o << "Group ID, Path ID, Excluded,Flag,Censored,Number of Animals In Clump, Extra Worm Count,"
		 "Unregistered Object Center X, Unregistered Object Center Y,"
		// "Offset from Path Magnitude, Offset within Registered Image Magnitude,"
		 "Registration Offset X, Registration Offset Y, Registration Offset Magnitude,"
		 "Absolute Time, Age Relative Time,"
		 "Machine-Annotated Movement State,By Hand Annotated Movement State,"
		 "Machine Death Relative Time, Machine Slow Movement Cessation Relative Time, Machine Fast Movement Cessation Relative Time,"
		 "By Hand Death Relative Time, By Hand Slow Movement Cessation Relative Time, By Hand Fast Movement Cessation Relative Time,"
		 "By Hand Death-Associated Contraction Time,"
		 "Unnormalized Movement Sum, Unnormalized Movement Sum Scaled by Worm Size, Movement Score, Denoised Movement Score,Movement Alternate Worm Sum, Change in Total Pixel Intensity Between Frames,Change in Average Normalized Pixel Intensity Between Frames, Movement Ratio,"
		 "Total Worm Area, Normalized Total Worm Area,Total Worm Intensity, Total Region Area, Total Region Intensity,"
		 "Normalized Worm Area, Normalized Worm Intensity, Normalized Change in Worm Intensity,"
		 "Total Alternate Worm Area,Total Alternate Worm Intensity,Saturated Registration, Local Region Maximum Movement Sum, Local Region Stationary Sum,"
		 "Local Region Position X, Local Region Position Y, Local Region Width, Local Region Height, Machine Error (days)";
	for (unsigned int i = 0; i < this->posture_quantification_extra_debug_field_names.size(); i++){
		o << "," << posture_quantification_extra_debug_field_names[i];
	}
}
void ns_analyzed_image_time_path::write_detailed_movement_quantification_analysis_data(const ns_region_metadata & m, const unsigned long group_id, const unsigned long path_id, std::ostream & o, const bool output_only_elements_with_hand,const bool abbreviated_time_series)const{
	if (output_only_elements_with_hand && by_hand_annotation_event_times[(int)ns_movement_cessation].period_end_was_not_observed)
		return;
	//find animal size one day before death
	double death_relative_time_to_normalize(0);
	double time_after_death_to_write_in_abbreviated(24*60*60);
	unsigned long end_index(0);
	long least_dt_to_death(LONG_MAX);
	double size_at_closest_distance(1);
	if(!state_intervals[(int)ns_movement_stationary].skipped){
		double death_time(0);
		if (by_hand_annotation_event_times[(int)ns_movement_stationary].period_end_was_not_observed)
			death_time = state_entrance_interval_time(state_intervals[(int)ns_movement_stationary]).best_estimate_event_time_for_possible_partially_unbounded_interval();
		else 
			death_time = by_hand_annotation_event_times[(int)ns_movement_stationary].period_end;
		
		const double time_to_match(death_time+death_relative_time_to_normalize);
		const double time_to_match_2(death_time+time_after_death_to_write_in_abbreviated);

		for (unsigned long k = 0; k < elements.size(); k++){
	
			const double dt(fabs(elements[k].absolute_time-time_to_match));
			if (dt < least_dt_to_death){
				least_dt_to_death = dt;
				size_at_closest_distance = element(k).measurements.total_worm_area;
			}
		}
		
		if (abbreviated_time_series){
			for (end_index = 0; end_index < elements.size(); end_index++){
				if (elements[end_index].absolute_time >= time_to_match_2)
					break;
			}
		}
	}
	if (!abbreviated_time_series)
		end_index = elements.size();

	for (unsigned long k = 1; k < end_index; k++){
		m.out_JMP_plate_identity_data_short(o);
		o << ",";
		o << group_id<<","<<path_id<<","
			<< ((censoring_and_flag_details.is_excluded() || censoring_and_flag_details.flag.event_should_be_excluded()) ?"1":"0")<< ",";
		if (censoring_and_flag_details.flag.specified())
				o << censoring_and_flag_details.flag.label_short;
			o << ","
			<< (censoring_and_flag_details.is_censored()?"1":"0") << ","
			<< censoring_and_flag_details.number_of_worms() << ","
			<< elements[k].number_of_extra_worms_observed_at_position << ","
			<< elements[k].region_offset_in_source_image().x + elements[k].worm_region_size().x/2 << ","
			<< elements[k].region_offset_in_source_image().y + elements[k].worm_region_size().y/2 << ","
			// "Offset from Path Magnitude, Offset within Registered Image Magnitude,"
		// "Registration Offset X, Registration Offset Y, Registration Offset Magnitude,"
		// "Absolute Time, Age Relative Time,"
			<< elements[k].measurements.registration_displacement.x << ","
			<< elements[k].measurements.registration_displacement.y << ","
			<< elements[k].measurements.registration_displacement.mag() << ","
			<< elements[k].absolute_time << ","
			<< ns_to_string_short((elements[k].absolute_time - m.time_at_which_animals_had_zero_age)/(60.0*60*24),3) << ","
			<< ns_movement_state_to_string(movement_state(elements[k].absolute_time)) << ","
			<< ns_movement_state_to_string(by_hand_movement_state(elements[k].absolute_time)) << ","
			<< ns_calc_rel_time_by_index(elements[k].absolute_time,state_intervals[(int)ns_movement_stationary],*this) << ","
			<< ns_calc_rel_time_by_index(elements[k].absolute_time,state_intervals[(int)ns_movement_posture],*this) << ","
			<< ns_calc_rel_time_by_index(elements[k].absolute_time,state_intervals[(int)ns_movement_slow],*this) << ","
			<< ns_output_interval_difference(elements[k].absolute_time,by_hand_annotation_event_times[(int)ns_movement_cessation]) << ","
			<< ns_output_interval_difference(elements[k].absolute_time,by_hand_annotation_event_times[(int)ns_translation_cessation]) << ","
			<< ns_output_interval_difference(elements[k].absolute_time,by_hand_annotation_event_times[(int)ns_fast_movement_cessation]) << ","
			<< ns_output_interval_difference(elements[k].absolute_time,by_hand_annotation_event_times[(int)ns_worm_death_posture_relaxation_termination]) << ",";

		if (elements[k].measurements.total_worm_area != 0){
		o   << elements[k].measurements.unnormalized_movement_sum << ","
			<< elements[k].measurements.unnormalized_movement_sum/(double)elements[k].measurements.total_worm_area << ",";
		}
		else o<< ",,";
		o	<< elements[k].measurements.movement_score << ","
			<< elements[k].measurements.denoised_movement_score << ","
			<< elements[k].measurements.movement_alternate_worm_sum << ","
			<< elements[k].measurements.change_in_total_worm_intensity << ","
			<< elements[k].measurements.change_in_average_normalized_worm_intensity << ","
			<< elements[k].measurements.new_movement_ratio() << ","
			<< elements[k].measurements.total_worm_area << ",";
			//size normalized to 1 exactly 1 day before death.
		if (least_dt_to_death > 60*60*12)
			o << ",";
		else o << elements[k].measurements.total_worm_area/size_at_closest_distance<< ",";
		o   << elements[k].measurements.total_intensity_within_worm_area << ","
			<< elements[k].measurements.total_region_area << ","
			<< elements[k].measurements.total_intensity_within_region << ","
			<< elements[k].measurements.normalized_worm_area << ","
			<< elements[k].measurements.normalized_total_intensity << ","
			<< elements[k].measurements.denoised_change_in_average_normalized_worm_intensity<< ","
			<< elements[k].measurements.total_alternate_worm_area << ","
			<< elements[k].measurements.total_intensity_within_alternate_worm << ","
			<< (elements[k].saturated_offset?"1":"0") <<  ","
			<< elements[k].measurements.local_maximum_movement_sum << ","
			<< elements[k].measurements.local_maximum_stationary_sum << ","
			<< elements[k].measurements.local_maximum_position.x << ","
			<< elements[k].measurements.local_maximum_position.y << ","
			<< elements[k].measurements.local_maximum_area.x << ","
			<< elements[k].measurements.local_maximum_area.y << ",";
		if ( state_intervals[(int)ns_movement_stationary].skipped)
			 o << "";
		else o << ns_output_interval_difference(this->state_entrance_interval_time(state_intervals[(int)ns_movement_stationary]).
													best_estimate_event_time_for_possible_partially_unbounded_interval(),
													by_hand_annotation_event_times[(int)ns_movement_cessation]);

		for (unsigned int i = 0; i < elements[k].measurements.posture_quantification_extra_debug_fields.size(); i++){
			o << "," << elements[k].measurements.posture_quantification_extra_debug_fields[i];
		}
		o << "\n";
	}
}

void ns_time_path_image_movement_analyzer::write_summary_movement_quantification_analysis_data(const ns_region_metadata & m, std::ostream & o)const{

	for (unsigned long i = 0; i < groups.size(); i++){
		for (unsigned long j = 0; j < groups[i].paths.size(); j++){	
			if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path() || groups[i].paths[j].entirely_excluded)
					continue;
			if (groups[i].paths[j].state_intervals.size() != ns_movement_number_of_states)
				throw ns_ex("ns_time_path_image_movement_analyzer::write_summary_movement_quantification_analysis_data()::Event Indicies not loaded properly!");
			groups[i].paths[j].write_summary_movement_quantification_analysis_data(m,i,j,o);
		}
	}
}


void ns_time_path_image_movement_analyzer::write_analysis_optimization_data(const std::vector<double> & thresholds, const std::vector<double> & hold_times, const ns_region_metadata & m,std::ostream & o) const{
	srand(0);
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){	
			if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path() || groups[i].paths[j].excluded() || !groups[i].paths[j].by_hand_data_specified())
					continue;
			groups[i].paths[j].write_analysis_optimization_data(generate_stationary_path_id(i,j),thresholds,hold_times,m,denoising_parameters_used,o);
		}
	}
}
void ns_time_path_image_movement_analyzer::write_detailed_movement_quantification_analysis_data(const ns_region_metadata & m, std::ostream & o, const bool only_output_elements_with_by_hand_data, const long specific_animal_id, const bool abbreviated_time_series)const{
	 
	for (unsigned long i = 0; i < groups.size(); i++){
		for (unsigned long j = 0; j < groups[i].paths.size(); j++){
			if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path() ||
				specific_animal_id != -1 && i != specific_animal_id || groups[i].paths[j].entirely_excluded)
				continue;
			if (groups[i].paths[j].state_intervals.size() != ns_movement_number_of_states)
				throw ns_ex("ns_time_path_image_movement_analyzer::write_movement_quantification_analysis_data()::Event Indicies not loaded properly!");
			groups[i].paths[j].write_detailed_movement_quantification_analysis_data(m,i,j,o,only_output_elements_with_by_hand_data,abbreviated_time_series);
		}
	}
}

/* When making animated graphs of time series data, we want to place a marker on the data point corresponding
	to the current time in the animation.  This marker moves accross data demonstrating the passage of time.
	When set_marker(time,graph) is called, the x_axis of the graph column is read to locate the time
	closest to the time specified as a function argument.  A marker is created in the graph object
	at the time closest to that requested.*/
class ns_marker_manager{
public:
	void set_marker(const unsigned long time,ns_graph & graph){
		unsigned int marker_position = 0;
		if (time == -1){
			for (unsigned int i = 0; i < graph.contents[marker_id].y.size(); i++){
				graph.contents[marker_id].y[i] = -1;
				return;
			}
		}
		unsigned int i;
		for (i = 0; i < graph.contents[x_axis_id].x.size(); i++){
			if (graph.contents[x_axis_id].x[i] >= (unsigned long)time)
				break;
			graph.contents[marker_id].y[i] = -1;
		}
		if (i == graph.contents[x_axis_id].x.size())
			return;
		graph.contents[marker_id].y[i] = graph.contents[y_axis_id].y[i];
		for (i = i+1; i < graph.contents[marker_id].y.size(); i++)
			graph.contents[marker_id].y[i] = -1;
	}
private:
	enum{y_axis_id = 0,x_axis_id=1,marker_id=2};
};
/*Movement is measured by collecting statistics on pixel changes over time.
ns_make_path_movement_graph produces a graph of those statistics
*/
void ns_make_path_movement_graph(const ns_analyzed_image_time_path & path,ns_graph & graph){

	const unsigned long number_of_measurements(path.element_count());
	if (number_of_measurements == 1)
		throw ns_ex("Not enough measurements!");

	ns_graph_object	movement_ratios(ns_graph_object::ns_graph_dependant_variable);
	movement_ratios.y.resize(number_of_measurements);
	ns_graph_object graph_x_axis(ns_graph_object::ns_graph_independant_variable);
	graph_x_axis.x.resize(number_of_measurements);

	//ns_graph_object cutoff_posture(ns_graph_object::ns_graph_dependant_variable),
	//			    cutoff_stationary(ns_graph_object::ns_graph_dependant_variable);

	//cutoff_posture.y.resize(number_of_measurements);
	//cutoff_stationary.y.resize(number_of_measurements);

	ns_graph_object transition_posture(ns_graph_object::ns_graph_vertical_line),
				    transition_stationary(ns_graph_object::ns_graph_vertical_line);

	ns_graph_object marker(ns_graph_object::ns_graph_dependant_variable);
	marker.y.resize(number_of_measurements,-1);

	

	for (unsigned int i = 0; i < path.element_count(); i++){
		movement_ratios.y[i] = path.element(i).measurements.new_movement_ratio();
		graph_x_axis.x[i] = path.element(i).absolute_time;
	//	cutoff_stationary.y[i] = ns_analyzed_image_time_path::stationary_cutoff_ratio();
	//	cutoff_posture.y[i] = ns_analyzed_image_time_path::posture_cutoff_ratio();
	}

	bool slow(true),posture(true);
//	transition_posture.y[0] = -1;
//	transition_stationary.y[0] = -1;
	for (unsigned int i = 1; i < path.element_count(); i++){
		ns_movement_state m(path.movement_state(path.element(i).absolute_time));
		if (slow && m == ns_movement_posture){
			transition_posture.x.push_back(path.element(i).absolute_time);
			transition_posture.y.push_back(movement_ratios.y[i]);
			//transition_posture.y[i]=0; 
			slow = false;
		}
	//	else transition_posture.y[i]=-1;
		

		if (posture && m == ns_movement_stationary){
			transition_stationary.x.push_back(path.element(i).absolute_time);
			transition_stationary.y.push_back(movement_ratios.y[i]);
			posture = false;
		}
	//	else transition_stationary.y[i]=-1;
	}

	marker.properties.line.draw = 0;
	marker.properties.area_fill.draw = 0;
	marker.properties.point.draw = true;
	marker.properties.point.color = marker.properties.area_fill.color;
	marker.properties.point.width = 3;
	marker.properties.point.edge_width = marker.properties.point.width/3;
	marker.properties.point.edge_color = ns_color_8(255,255,175);
	marker.properties.draw_negatives = false;

	transition_posture.properties.point.draw = false;
	transition_posture.properties.line.draw = true;
	transition_posture.properties.line.width = 2;
	transition_posture.properties.line_hold_order = ns_graph_properties::ns_first;
	transition_posture.properties.draw_vertical_lines = ns_graph_properties::ns_no_line;
	transition_posture.properties.line.color = ns_color_8(175,0,0);

	//cutoff_stationary.properties = transition_posture.properties;
	//cutoff_stationary.properties =  ns_color_8(175,0,0);;
	//cutoff_posture.properties = transition_stationary.properties;
	//cutoff_posture.properties.line.color = ns_color_8(0,175,0);
	//transition_posture.properties = cutoff_posture.properties;
	transition_posture.properties.draw_vertical_lines = ns_graph_properties::ns_full_line;
	transition_posture.properties.line_hold_order = ns_graph_properties::ns_zeroth_centered;
	transition_posture.properties.draw_negatives = false;
	transition_posture.properties.line.color = ns_color_8(0,255,0);

	transition_stationary.properties = transition_posture.properties;
	transition_stationary.properties.draw_vertical_lines = ns_graph_properties::ns_full_line;
	transition_stationary.properties.line_hold_order = ns_graph_properties::ns_zeroth_centered;
	transition_stationary.properties.draw_negatives = false;
	transition_stationary.properties.line.color = ns_color_8(255,0,0);

	movement_ratios.properties.point.draw = false;
	movement_ratios.properties.line.draw = true;
	movement_ratios.properties.line.width = 2;
	movement_ratios.properties.line_hold_order = ns_graph_properties::ns_first;
	movement_ratios.properties.draw_vertical_lines = ns_graph_properties::ns_no_line;
	movement_ratios.properties.line.color=ns_color_8(125,125,125);

	graph.x_axis_properties.line.color=ns_color_8(175,175,175);
	graph.x_axis_properties.text.draw =false;
	graph.x_axis_properties.point.color=ns_color_8(175,175,175);
	graph.x_axis_properties.area_fill.color=ns_color_8(0,0,0);
	graph.x_axis_properties.draw_tick_marks = false;
	graph.y_axis_properties = 
		graph.area_properties = 
		graph.title_properties = 
		graph.x_axis_properties;



	graph.contents.push_back(movement_ratios);
	graph.contents.push_back(graph_x_axis);
	graph.contents.push_back(marker);
	//graph.contents.push_back(cutoff_stationary);
//	graph.contents.push_back(cutoff_posture);
	//graph.contents.push_back(transition_stationary);
	//graph.contents.push_back(transition_posture);
	ns_graph_axes axes;
	graph.set_graph_display_options("",axes);

}
/*
void ns_analyzed_image_time_path::output_image_movement_summary(std::ostream & o){

	o << "Time,Movement Abs,Stationary,Ratio,State\n";
	for (unsigned int i = 0; i < elements.size(); i++){
		float r(elements[i].movement_ratio());
		o << elements[i].absolute_time << "," <<
			elements[i].
			<< elements[i].movement - elements[i].movement_alternate_worm_sum << ","
			<< elements[i].stationary_sum << ","
			<< r << ","
			<< this->movement_state(elements[i].absolute_time) << "\n";
	}
}*/


ns_death_time_annotation_time_interval ns_analyzed_image_time_path::state_entrance_interval_time(const ns_movement_state_observation_boundary_interval & e) const{
	if (e.skipped)
		throw ns_ex("state_entrance_interval_time()::Requesting state entrance time for skipped state!");
	if (e.entrance_interval.interval_occurs_after_observation_interval)
		throw ns_ex("state_entrance_interval_time()::Requesting the entrance time of an interval reported to occur after observation interval!");
	if (e.entrance_interval.interval_occurs_before_observation_interval)
		return time_path_limits.interval_before_first_observation;

	if (e.entrance_interval.period_start_index < 0 || e.entrance_interval.period_start_index >= elements.size() ||
		e.entrance_interval.period_end_index < 0 || e.entrance_interval.period_end_index >= elements.size())
		throw ns_ex("ns_analyzed_image_time_path::state_entrance_interval_time()::Invalid time: ") << e.entrance_interval.period_start_index << " out of a path with " << elements.size() << " elements";
		
	return ns_death_time_annotation_time_interval(elements[e.entrance_interval.period_start_index].absolute_time,
		elements[e.entrance_interval.period_end_index].absolute_time);
}
/*
ns_death_time_annotation_time_interval ns_analyzed_image_time_path::state_exit_interval_time(const ns_movement_state_observation_boundary_interval & e) const{
	if (e.skipped) throw ns_ex("state_exit_interval_time()::Requesting state exit time for skipped state!");
	if (e.exit_interval.interval_occurs_before_observation_interval)
		return time_path_limits.interval_before_first_observation;
	if (e.exit_interval.interval_occurs_after_observation_interval)	
		throw ns_ex("state_entrance_interval_time()::Requesting the exit time of an interval reported to occur after observation interval!");

	if (e.exit_interval.period_start_index < 0 || e.exit_interval.period_start_index >= elements.size() ||
		e.exit_interval.period_end_index < 0 || e.exit_interval.period_end_index >= elements.size())
		throw ns_ex("ns_analyzed_image_time_path::state_entrance_interval_time()::Invalid time!");
		
	return ns_death_time_annotation_time_interval(elements[e.exit_interval.period_start_index].absolute_time,
													elements[e.exit_interval.period_end_index].absolute_time);
}
*/

void ns_analyzed_image_time_path::add_death_time_events_to_set(ns_death_time_annotation_set & set) const{
	set.add(death_time_annotation_set);
}

long ns_find_last_valid_observation_index(const long index,const ns_analyzed_image_time_path::ns_element_list & e){
	for (long i = index-1; i >= 0; i--){
		if (!e[i].excluded)
			return i;
	}
	return -1;
}
//Welcome to the dark innards of the lifespan machine!
//
//detect_death_times_and_generate_annotations_from_movement_quantification() is the part of the machine
//that turns observations of movement into death time annotations; there's a lot of detail here
//so bear with me as I attempt to explain.
//Note that how those annotations are handled (and aggregated into mortality data) is up to downstream users
//Annotations are a description of what happened when, nothing more.
//
//The specified ns_analyzed_image_time_path_death_time_estimator 
//analyzes the movement quantification and identifies the frames at which
//the animal stops moving slowly and when it stops changing posture (i.e it dies).
//Complexity is introduced because animals do not changes states at a specific
//time, they change states during the interval between observations.
//So we need to convert the frame indicies produced by ns_analyzed_image_time_path_death_time_estimator ()
//into observation periods--the period between the last observation during which the animal was in one state
//and the first observation in which it was in the next.
//This would be easy if all worms were observed at all times, but 
//since worms can be missing for various amounts of time and observations may be explicitly excluded
//we need to be careful about finding the right observation periods.
//
//Prior to producing annotations, for each possible state we build description of the interval in which
//that state was occupiued; e.g the pair of observations in which an animal enters a state and 
//and the pair of observations in which the animal exits the state.
//These intervals are saved in the state_intervals structure for later use by other time_path_image_analyzer member functions
//and also used to generate annotations
//
//It is possible that animals may fail to be observed entering or exiting certain states.
//If an animal is in a state from the beginning of the path the state interval specificaiton  has its 
//ns_movement_state_observation_boundary_interval::entrance_interval::interval_occurs_before_observation_interval flag set
//Similarly, states that the animal is never observed to leave have their 
//ns_movement_state_observation_boundary_interval::exit_interval::interval_occurs_after_observation_interval flag set

//The state intervals are used to generate event annotations.
//In the case where the state begins before the start of the pathis assumed that it entered that state
//during the interval ending with the first observation in the path. 
//This means, for examples, animals who are identified as stationary at the first frame of the path are reported to have done so
//in the interval immediately before the path starts.
//If the case where the path starts at the beginning of the observation period (there is no 
//previous observation interval), then the ns_death_time_annotation_time_interval::period_start_was_not_observed flag
//is set on the apropriate annotation.  These flags are set during the initialization of the time path group, eg
//ns_analyzed_image_time_path_group::ns_analyzed_image_time_path_group()
	
//
//There is no problem handling events that continue past the end of the path;
//we don't use this information for anything and do not output any annotations about it.

void ns_analyzed_image_time_path::detect_death_times_and_generate_annotations_from_movement_quantification(const ns_stationary_path_id & path_id, const ns_analyzed_image_time_path_death_time_estimator * movement_death_time_estimator,ns_death_time_annotation_set & set, const unsigned long last_time_point_in_analysis){
	
	//if (path_id.group_id == 5)
	//	cerr << "RA";
	state_intervals.clear();
	state_intervals.resize((int)ns_movement_number_of_states);
	by_hand_annotation_event_times.resize((int)ns_number_of_movement_event_types,ns_death_time_annotation_time_interval::unobserved_interval());

	unsigned long current_time(ns_current_time());
	set.clear();
	if (elements.size() == 0) return;

	ns_movement_state_time_interval_indicies first_valid_element_id(elements.size(),elements.size()),
				  last_valid_element_id(0,0);
	for (unsigned int i = 0; i < elements.size(); i++)
		if (!elements[i].excluded && !elements[i].element_before_fast_movement_cessation){
			//find first measurment
			if (first_valid_element_id.period_start_index >= i)
				first_valid_element_id.period_start_index = i;
			else
				//find the second measurement
			if (first_valid_element_id.period_end_index > i)
				first_valid_element_id.period_end_index = i;

			//find the last measurement
			if (last_valid_element_id.period_end_index < i){
				//set the measurement previous to the last measurement
				last_valid_element_id.period_start_index =
					last_valid_element_id.period_end_index;
				last_valid_element_id.period_end_index = i;
			}
		}
		if (first_valid_element_id.period_start_index+1 >= elements.size())
		return;

	//if the experiment is cropped before the current path ends, then we don't add a stationary worm is lost event.
	//unsigned long actual_time_of_first_measurement_after_path_end(time_path_limits.interval_after_last_observation.period_end);
	//if (last_valid_element_id.period_end+1 < elements.size())
	//	actual_time_of_first_measurement_after_path_end = 0;

	//if the path has been declared as not having enough information
	//for annotation, register it as so.
	if (this->is_low_density_path()){
		unsigned long end_index(first_valid_element_id.period_end_index);
		if (end_index > elements.size())
			throw ns_ex("Invalid end index");
		if (end_index == elements.size())
			end_index = elements.size()-1;
		set.add(ns_death_time_annotation(ns_no_movement_event,
			0,region_info_id,
			ns_death_time_annotation_time_interval(elements[first_valid_element_id.period_start_index].absolute_time,
													elements[end_index].absolute_time),
			elements[end_index].region_offset_in_source_image(),
			elements[end_index].worm_region_size(),
			ns_death_time_annotation::ns_not_excluded,			//filling in the gaps of these things work really well! Let the user exclude them in the worm browser
			ns_death_time_annotation_event_count(1+elements[end_index].number_of_extra_worms_observed_at_position,0),
			current_time,ns_death_time_annotation::ns_lifespan_machine,
			(elements[end_index].part_of_a_multiple_worm_disambiguation_group)?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
			path_id,true,elements[end_index].inferred_animal_location,"low_density"));
	}
	ns_time_path_posture_movement_solution movement_state_solution(movement_death_time_estimator->operator()(this,true));

	//Some movement detection algorithms need a significant amount of time after an animal has died
	//to detect its death.  So, if we are going to censor an animal at the end of the experiment,
	//we need to ask the movement detection about the latest time such an event can occur.
	const unsigned long last_possible_death_time(movement_death_time_estimator->latest_possible_death_time(this,last_time_point_in_analysis));


	const double loglikelihood_of_solution(movement_state_solution.loglikelihood_of_solution);
	const string reason_to_be_censored(movement_state_solution.reason_for_animal_to_be_censored);

	ns_movement_state_observation_boundary_interval slow_moving_interval_including_missed_states,
														posture_changing_interval_including_missed_states,
														dead_interval_including_missed_states;

	slow_moving_interval_including_missed_states.longest_observation_gap_within_interval = movement_state_solution.slowing.longest_observation_gap_within_interval;
	posture_changing_interval_including_missed_states.longest_observation_gap_within_interval = movement_state_solution.moving.longest_observation_gap_within_interval;
	dead_interval_including_missed_states.longest_observation_gap_within_interval = movement_state_solution.dead.longest_observation_gap_within_interval;
	
	
	unsigned long longest_skipped_interval_before_death = slow_moving_interval_including_missed_states.longest_observation_gap_within_interval;
	if (longest_skipped_interval_before_death < posture_changing_interval_including_missed_states.longest_observation_gap_within_interval)
		longest_skipped_interval_before_death = posture_changing_interval_including_missed_states.longest_observation_gap_within_interval;	
	
	{
		ns_movement_state_observation_boundary_interval slow_moving_interval,
														posture_changing_interval,
														dead_interval;

		//the movement detection algorithms give us the first frame in which the animal was observed in each state.
		//We need to conver these into *intervals* between frames during which the transition occurred.
		//This is because events may have occurred at any time during the interval and we don't want to assume
		//the event occurred at any specific time (yet!)
		//We'll resolve this observational ambiguity later.
	
		ns_movement_state_time_interval_indicies frame_before_first(first_valid_element_id);
		if (first_valid_element_id.period_start_index == 0){
			frame_before_first.interval_occurs_before_observation_interval = true;
			frame_before_first.period_start_index = -1;
		}
		else frame_before_first.period_start_index--;
		frame_before_first.interval_occurs_after_observation_interval = false;
		frame_before_first.period_end_index--;

		ns_movement_state_time_interval_indicies frame_after_last(elements.size()-1,elements.size());
		frame_after_last.interval_occurs_after_observation_interval = true;


		if (movement_state_solution.moving.skipped){
			if (movement_state_solution.slowing.skipped &&
				movement_state_solution.dead.skipped)
				throw ns_ex("ns_analyzed_image_time_path::detect_death_times_and_generate_annotations_from_movement_quantification()::Movement death time estimator skipped all states!");
			//if we skip slow moving, that means we start changing posture or death, which means
			//we transition out of fast movement before the first frame of the path
			slow_moving_interval.skipped = true;
		}
		else{
			slow_moving_interval.skipped = false;
			slow_moving_interval.entrance_interval = frame_before_first;
			//if all following states are skipped, this state continues past the end of the observation interval
			if (movement_state_solution.slowing.skipped && movement_state_solution.dead.skipped)
				slow_moving_interval.exit_interval = frame_after_last;
			else{
				slow_moving_interval.exit_interval.period_end_index = movement_state_solution.moving.end_index;
				slow_moving_interval.exit_interval.period_start_index = ns_find_last_valid_observation_index(movement_state_solution.moving.end_index,elements);
			}
		}
		if (movement_state_solution.slowing.skipped){
			posture_changing_interval.skipped = true;
		}
		else{
			posture_changing_interval.skipped = false;
			//if the animal is never observed to be slow moving, it has been changing posture since before the observation interval
			if (slow_moving_interval.skipped)
				posture_changing_interval.entrance_interval = frame_before_first;
			else 
				posture_changing_interval.entrance_interval = slow_moving_interval.exit_interval;
			//if the animal never dies, it continues to change posture until the end of observation interval
			if (movement_state_solution.dead.skipped)
				posture_changing_interval.exit_interval	= frame_after_last;
			else{
				posture_changing_interval.exit_interval.period_end_index = movement_state_solution.slowing.end_index;
				posture_changing_interval.exit_interval.period_start_index = ns_find_last_valid_observation_index(movement_state_solution.slowing.end_index,elements);
			}
		}
		if (movement_state_solution.dead.skipped){
			dead_interval.skipped = true;
		}
		else{
			dead_interval.skipped = false;
			if (posture_changing_interval.skipped && slow_moving_interval.skipped)
				dead_interval.entrance_interval = frame_before_first;
			else if (posture_changing_interval.skipped)
				dead_interval.entrance_interval = slow_moving_interval.exit_interval;
			else
				dead_interval.entrance_interval = posture_changing_interval.exit_interval;

			dead_interval.exit_interval = frame_after_last;
		}
	
		state_intervals[ns_movement_slow] = slow_moving_interval;
		state_intervals[ns_movement_posture] = posture_changing_interval;
		state_intervals[ns_movement_stationary] = dead_interval;

	
		//ok we have the correct state intervals.  BUT we need to change them around because we know that animals have to pass through 
		//fast to slow movement to posture, and posture to death.  If the movement detection algorithms didn't find any states
		//that's because the animals went through too quickly.  So we make *new* intervals for this purpose with those assumptions in mind
		slow_moving_interval_including_missed_states = slow_moving_interval;
		posture_changing_interval_including_missed_states = posture_changing_interval;
		dead_interval_including_missed_states = dead_interval;
	
		//to make it into a path, the animals *have* to have slowed down to slow moving.
		if (slow_moving_interval.skipped){
			slow_moving_interval_including_missed_states.skipped = false;
			slow_moving_interval_including_missed_states.entrance_interval = frame_before_first;
			if (!posture_changing_interval.skipped)
				slow_moving_interval_including_missed_states.exit_interval = posture_changing_interval.entrance_interval;
			else if (!dead_interval.skipped)
				slow_moving_interval_including_missed_states.exit_interval = dead_interval.entrance_interval;
			else throw ns_ex("Movement estimator reported slow movent as having been skipped when all later states had also been skipped!");
		}
		//we only know that the posture changing interval *had* to have occurred if the animal was ultimately seen to have died.
		if (posture_changing_interval.skipped && !dead_interval.skipped){
			posture_changing_interval_including_missed_states.skipped = false;
			//if both slow and posture are skipped, they both occur the frame before the first
			if (slow_moving_interval.skipped)
				posture_changing_interval_including_missed_states = slow_moving_interval_including_missed_states;
			else{
				//otherwise the posture changing interval was sandwhiched between fast movement and death.
				posture_changing_interval_including_missed_states.entrance_interval = dead_interval.entrance_interval;
				posture_changing_interval_including_missed_states.exit_interval = dead_interval.entrance_interval;
			}
		}
		//dead animals can be skipped without any editing, as we'll never be able to confirm that it happened unless we observed it.
	}

	//if the path has extra worms at least 25% of the points leading up to it's death
		//mark the path as containing that extra worm
	unsigned long number_of_extra_worms_in_path(0),
		total_observations(0);
	bool part_of_a_multiple_worm_disambiguation_group(false);
	{
		unsigned long stop_i(last_valid_element_id.period_end_index);
		if (!dead_interval_including_missed_states.skipped)
			stop_i=dead_interval_including_missed_states.entrance_interval.period_end_index;
	
		unsigned long total_number_of_extra_worms(0),total_mult_worm_d(0);
		for (unsigned int i = 0; i < stop_i; i++){
			total_number_of_extra_worms+=elements[i].number_of_extra_worms_observed_at_position;
			total_mult_worm_d+=elements[i].part_of_a_multiple_worm_disambiguation_group?1:0;
			total_observations++;
		}
		if (total_observations == 0){
			number_of_extra_worms_in_path = 0;
			part_of_a_multiple_worm_disambiguation_group = 0;
		}
		else{
			number_of_extra_worms_in_path = (unsigned long)(floor(total_number_of_extra_worms/(float)(total_observations)+.75));
			part_of_a_multiple_worm_disambiguation_group = floor(total_mult_worm_d/(float)(total_observations)+.75) != 0;
		}
	}
	const bool part_of_a_full_trace(!dead_interval_including_missed_states.skipped);

	//Since the animal has slowed down enough to count as a path, we mark
	//the interval during which it slowed down as one before the first frame of the path.
	//ns_death_time_annotation_time_interval slow_movement_entrance_interval;
	//if (slow_moving_interval.skipped)
	//	slow_movement_entrance_interval = time_path_limits.interval_before_first_observation;
	//else slow_movement_entrance_interval = state_entrance_interval_time(slow_moving_interval);
	
	//if (path_id.group_id == 28)
	//	cerr << "MA";
	ns_death_time_annotation::ns_exclusion_type exclusion_type(ns_death_time_annotation::ns_not_excluded);
	if (!reason_to_be_censored.empty()){
		exclusion_type = ns_death_time_annotation::ns_censored;
	}
	set.add(
		ns_death_time_annotation(ns_fast_movement_cessation,
		0,region_info_id,
		state_entrance_interval_time(slow_moving_interval_including_missed_states),
		elements[first_valid_element_id.period_start_index].region_offset_in_source_image(),  //register the position of the object at that time point
		elements[first_valid_element_id.period_start_index].worm_region_size(),
		exclusion_type,
		ns_death_time_annotation_event_count(1+number_of_extra_worms_in_path,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
		(part_of_a_multiple_worm_disambiguation_group)?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
		path_id,part_of_a_full_trace,elements[first_valid_element_id.period_start_index].inferred_animal_location,reason_to_be_censored,loglikelihood_of_solution,longest_skipped_interval_before_death));
	
	//observations are made at specific times (i.e. we see a fast moving worm at this time)
	for (unsigned int i = slow_moving_interval_including_missed_states.entrance_interval.period_end_index; i < slow_moving_interval_including_missed_states.entrance_interval.period_end_index; i++){
		if (elements[i].excluded) continue;

		set.add(
			ns_death_time_annotation(ns_slow_moving_worm_observed,
			0,region_info_id,
			ns_death_time_annotation_time_interval(elements[i].absolute_time,elements[i].absolute_time),
			elements[i].region_offset_in_source_image(),
			elements[i].worm_region_size(),exclusion_type,
			ns_death_time_annotation_event_count(1+elements[i].number_of_extra_worms_observed_at_position,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
			elements[i].part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
			path_id,part_of_a_full_trace,elements[i].inferred_animal_location,reason_to_be_censored,loglikelihood_of_solution,longest_skipped_interval_before_death)
			);
	}
	
	if (posture_changing_interval_including_missed_states.skipped && dead_interval_including_missed_states.skipped)
		return;
	//ns_death_time_annotation_time_interval posture_changing_entrance_interval;
	//if animals skip slow moving, we assume they transition from fast moving to dead within one interval
	//if (posture_changing_interval.skipped){
	//	posture_changing_entrance_interval = state_entrance_interval_time(dead_interval);
	//else if (slow_moving_interval.skipped)
	//	posture_changing_entrance_interval = state_entrance_interval_time(posture_changing_interval);
	if (posture_changing_interval_including_missed_states.entrance_interval.period_start_index >
		posture_changing_interval_including_missed_states.entrance_interval.period_end_index)
		throw ns_ex("Death start interval boundaries appear to be reversed:") << posture_changing_interval_including_missed_states.entrance_interval.period_end_index << " vs "
			<< posture_changing_interval_including_missed_states.entrance_interval.period_end_index;
	if (posture_changing_interval_including_missed_states.entrance_interval.interval_occurs_after_observation_interval)
		throw ns_ex("Encountered an unskipped start interval for posture changing that occurs on the last element of a timeseries");
	if (posture_changing_interval_including_missed_states.entrance_interval.period_end_index >= elements.size())
		throw ns_ex("Encountered an unskipped start interval for posture changing with an invalid end point");
		
	set.add(
		ns_death_time_annotation(ns_translation_cessation,
		0,region_info_id,
		state_entrance_interval_time(posture_changing_interval_including_missed_states),
		elements[posture_changing_interval_including_missed_states.entrance_interval.period_end_index].region_offset_in_source_image(),  //register the position of the object at that time point
		elements[posture_changing_interval_including_missed_states.entrance_interval.period_end_index].worm_region_size(),
		exclusion_type,
		ns_death_time_annotation_event_count(number_of_extra_worms_in_path+1,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
		part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
		path_id,part_of_a_full_trace,elements[posture_changing_interval_including_missed_states.entrance_interval.period_end_index].inferred_animal_location,reason_to_be_censored,loglikelihood_of_solution,longest_skipped_interval_before_death));
	
	
	for (unsigned int i = posture_changing_interval_including_missed_states.entrance_interval.period_end_index; i < posture_changing_interval_including_missed_states.exit_interval.period_end_index; i++){
		if (elements[i].excluded) continue;
		set.add(
			ns_death_time_annotation(ns_posture_changing_worm_observed,
			0,region_info_id,
			ns_death_time_annotation_time_interval(elements[i].absolute_time,elements[i].absolute_time),
			elements[i].region_offset_in_source_image(),  //register the position of the object at that time point
			elements[i].worm_region_size(),
			exclusion_type,
			ns_death_time_annotation_event_count(1+elements[i].number_of_extra_worms_observed_at_position,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
			elements[i].part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
			path_id,part_of_a_full_trace,elements[i].inferred_animal_location,reason_to_be_censored,loglikelihood_of_solution,longest_skipped_interval_before_death));
	}
		
	if (dead_interval_including_missed_states.skipped){
		//we allow animals who are alive at the experiment, and confirmed as real animals (using the storyboard) to be exported
		//as censored.
		bool remains_alive(false);
		unsigned long last_observation_index(posture_changing_interval_including_missed_states.exit_interval.period_end_index);
		if (posture_changing_interval_including_missed_states.exit_interval.interval_occurs_after_observation_interval)
			last_observation_index = posture_changing_interval_including_missed_states.exit_interval.period_start_index;
	
		if (elements[last_observation_index].absolute_time > last_possible_death_time){
				unsigned long i1;
				for (unsigned int k = 0; k < elements.size(); k++){
					i1 = k;
					if (elements[k].absolute_time > last_possible_death_time){
						break;
					}
				}
				ns_death_time_annotation_time_interval interval(0, elements[i1].absolute_time);
				if (i1 > 0)
					interval.period_start = elements[i1-1].absolute_time;
				else interval.period_start_was_not_observed = true;

				set.add(
					ns_death_time_annotation(ns_movement_cessation,
					0,region_info_id,
					interval,
					elements[i1].region_offset_in_source_image(),
					elements[i1].worm_region_size(),
					ns_death_time_annotation::ns_censored_at_end_of_experiment,
					ns_death_time_annotation_event_count(1+number_of_extra_worms_in_path,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
					part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
					path_id,part_of_a_full_trace,
					elements[i1].inferred_animal_location,
					"Alive at experiment end",loglikelihood_of_solution,longest_skipped_interval_before_death));
	
	}

		return;
	}

	
	if (dead_interval_including_missed_states.entrance_interval.period_start_index >=
		dead_interval_including_missed_states.entrance_interval.period_end_index)
		throw ns_ex("Death start interval boundaries appear to be equal or reversed:") << dead_interval_including_missed_states.entrance_interval.period_end_index << " vs "
			<< dead_interval_including_missed_states.exit_interval.period_end_index;

	if (dead_interval_including_missed_states.entrance_interval.interval_occurs_after_observation_interval)
		throw ns_ex("Encountered an unskipped start interval for death that occurs on the last element of a timeseries");
	if (dead_interval_including_missed_states.entrance_interval.period_end_index >= elements.size())
		throw ns_ex("Encountered an unskipped start interval for death with an invalid end point");
		
	set.add(
		ns_death_time_annotation(ns_movement_cessation,
		0,region_info_id,
		ns_death_time_annotation_time_interval(state_entrance_interval_time(dead_interval_including_missed_states)),
		elements[dead_interval_including_missed_states.entrance_interval.period_end_index].region_offset_in_source_image(),
		elements[dead_interval_including_missed_states.entrance_interval.period_end_index].worm_region_size(),
		exclusion_type,
		ns_death_time_annotation_event_count(1+number_of_extra_worms_in_path,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
		part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
		path_id,part_of_a_full_trace,elements[dead_interval_including_missed_states.entrance_interval.period_end_index].inferred_animal_location,reason_to_be_censored,loglikelihood_of_solution,longest_skipped_interval_before_death));
	
	for (unsigned int i = dead_interval_including_missed_states.entrance_interval.period_end_index; i < dead_interval_including_missed_states.exit_interval.period_end_index; i++){
		if (elements[i].excluded) continue;
		set.add(
			ns_death_time_annotation(ns_stationary_worm_observed,
			0,region_info_id,
			ns_death_time_annotation_time_interval(elements[i].absolute_time,elements[i].absolute_time),
			elements[i].region_offset_in_source_image(),
			elements[i].worm_region_size(),
			exclusion_type,
			ns_death_time_annotation_event_count(elements[i].number_of_extra_worms_observed_at_position+1,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
			elements[i].part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
			path_id,part_of_a_full_trace,elements[i].inferred_animal_location,reason_to_be_censored,loglikelihood_of_solution,longest_skipped_interval_before_death));
	}

	//const ns_death_time_annotation_time_interval death_exit_time(ns_death_time_annotation_time_interval(state_exit_interval_time(dead_interval)));

	//if the path ends before the end of the plate's observations
	//output an annotation there.
	if (!time_path_limits.interval_after_last_observation.period_end_was_not_observed){

		set.add(
			ns_death_time_annotation(ns_stationary_worm_disappearance,
			0,region_info_id,
			time_path_limits.interval_after_last_observation,
			elements[last_valid_element_id.period_end_index].region_offset_in_source_image(),
			elements[last_valid_element_id.period_end_index].worm_region_size(),
			ns_death_time_annotation::ns_not_excluded,
			ns_death_time_annotation_event_count(1+number_of_extra_worms_in_path,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
			part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
			path_id,part_of_a_full_trace,elements[last_valid_element_id.period_end_index].inferred_animal_location,"",loglikelihood_of_solution));
	}
}

void ns_analyzed_image_time_path::analyze_movement(const ns_analyzed_image_time_path_death_time_estimator * movement_death_time_estimator,const ns_stationary_path_id & path_id, const unsigned long last_timepoint_in_analysis){
	death_time_annotation_set.clear();
	
	detect_death_times_and_generate_annotations_from_movement_quantification(path_id,movement_death_time_estimator,death_time_annotation_set,last_timepoint_in_analysis);
}
bool inline ns_state_match(const unsigned long t,const ns_movement_state_observation_boundary_interval & i, const ns_analyzed_image_time_path & p){
	if (i.skipped)
		return false;
	const ns_death_time_annotation_time_interval interval(p.state_entrance_interval_time(i));
	if (!interval.period_start_was_not_observed &&
		t < interval.period_start)
		return false;
	if (!interval.period_end_was_not_observed &&
		t >= interval.period_end)
		return false;
	return true;
}
ns_movement_state ns_analyzed_image_time_path::movement_state(const unsigned long & t) const{
	//if (this->is_not_stationary())
//		return ns_movement_fast;
	if (this->is_low_density_path())
		return ns_movement_machine_excluded;

	//ns_event_index_list::const_iterator p;
	if (ns_state_match(t,state_intervals[ns_movement_fast],*this))
		return ns_movement_fast;
	
	if (ns_state_match(t,state_intervals[ns_movement_slow],*this))
		return ns_movement_slow;

	if (ns_state_match(t,state_intervals[ns_movement_posture],*this))
		return ns_movement_posture;
	
	if (ns_state_match(t,state_intervals[ns_movement_stationary],*this))
		return ns_movement_stationary;

	if (ns_state_match(t,state_intervals[ns_movement_death_posture_relaxation],*this))
		return ns_movement_death_posture_relaxation;
	return ns_movement_not_calculated;
}
int ns_greater_than_time(const long a, const long t){
	if (t == -1)
		return -1;
	return a < t;
}
bool ns_analyzed_image_time_path::by_hand_data_specified() const{
	return !by_hand_annotation_event_times.empty() &&
		(!by_hand_annotation_event_times[ns_movement_cessation].period_end_was_not_observed 
			|| !by_hand_annotation_event_times[(int)ns_translation_cessation].period_end_was_not_observed  
			|| !by_hand_annotation_event_times[(int)ns_fast_movement_cessation].period_end_was_not_observed 
			);
}
ns_death_time_annotation_time_interval ns_analyzed_image_time_path::by_hand_death_time() const{
	if (!by_hand_annotation_event_times[ns_movement_cessation].period_end_was_not_observed)
		return by_hand_annotation_event_times[ns_movement_cessation];
	else return ns_death_time_annotation_time_interval::unobserved_interval();
}
ns_movement_state ns_analyzed_image_time_path::by_hand_movement_state( const unsigned long & t) const{
	
	if (!by_hand_annotation_event_times[ns_movement_cessation].period_end_was_not_observed &&
		t >= by_hand_annotation_event_times[ns_movement_cessation].period_end){
		if (!by_hand_annotation_event_times[ns_worm_death_posture_relaxation_termination].period_end_was_not_observed &&
			t < by_hand_annotation_event_times[ns_worm_death_posture_relaxation_termination].period_end)
			return ns_movement_death_posture_relaxation;
		else
			return ns_movement_stationary;
	}
	if (by_hand_annotation_event_times[(int)ns_translation_cessation].period_end_was_not_observed &&
		t >= by_hand_annotation_event_times[(int)ns_translation_cessation].period_end)
		return ns_movement_posture;

	if (by_hand_annotation_event_times[(int)ns_fast_movement_cessation].period_end_was_not_observed){
		if(t >= by_hand_annotation_event_times[(int)ns_fast_movement_cessation].period_end)
			return ns_movement_slow;
		else return ns_movement_fast;
	}

	return ns_movement_not_calculated;
}
void ns_time_path_image_movement_analyzer::produce_death_time_annotations(ns_death_time_annotation_set & set) const{
	
	set.clear();
	for (unsigned long j = 0; j < groups.size(); j++){
		for (unsigned long k = 0; k < groups[j].paths.size(); k++){
			groups[j].paths[k].add_death_time_events_to_set(set);
		}
	}

	//add animals that are fast moving at the last time point
	set.add(extra_non_path_events);
	
}

void ns_analyzed_image_time_path_group::clear_images(){
	for (unsigned int i = 0; i < paths.size(); i++)
		for (unsigned int j = 0; j < paths[i].elements.size(); j++){
			paths[i].elements[j].clear_movement_images();
			paths[i].elements[j].clear_path_aligned_images();

		}

}

//transfers a line from a rbg image to a grayscale one.
class ns_output_subline{
public:
	template<class ns_component, class ns_component_2>
		inline void operator()(const ns_image_whole<ns_component> & source, 
			const unsigned long & source_x_offset, 
			const unsigned long & dest_x_offset,
			const unsigned long & width, 
			const unsigned long &source_y, 
			const unsigned long &dest_y,
			ns_image_whole<ns_component_2> & dest,const unsigned long & channel){
			if (width+dest_x_offset >= dest.properties().width)
				throw ns_ex("ns_output_subline()::X overflow!!");
			if (dest_y >= dest.properties().height)
				throw ns_ex("ns_output_subline()::X overflow!!");
			for (unsigned int x = 0; x < dest_x_offset; x++)
				dest[dest_y][x] = 0;
			for (unsigned int x = 0; x < width; x++)
				dest[dest_y][x+dest_x_offset] = (ns_component_2)source[source_y][3*(x+source_x_offset)+channel];
			for (unsigned int x = width+dest_x_offset; x < dest.properties().width; x++)
				dest[dest_y][x] = 0;
		}	
	template<class ns_component>
		inline void output_specific_value(const ns_image_whole<ns_component> & source, 
			const unsigned long & source_x_offset, 
			const unsigned long & dest_x_offset,
			const unsigned long & width, 
			const unsigned long &source_y, 
			const unsigned long &dest_y,
			ns_path_aligned_image_set * dest,const unsigned long & channel, const ns_component region_val,const ns_component worm_val){

			for (unsigned int x = 0; x < dest_x_offset; x++)
				dest->set_thresholds(dest_y,x,0,0);
			for (unsigned int x = 0; x < width; x++)
				dest->set_thresholds(dest_y,x+dest_x_offset,
				source[source_y][3*(x+source_x_offset)+channel] == region_val,
				source[source_y][3*(x+source_x_offset)+channel] == worm_val
			);
			for (unsigned int x = width+dest_x_offset; x < dest->image.properties().width; x++)
				dest->set_thresholds(dest_y,x,0,0);
		}
};



bool ns_analyzed_image_time_path::region_image_is_required(const unsigned long time, const bool interpolated, const bool moving_backward){
	for (unsigned int k = 0; k < elements.size(); k++){
		if (elements[k].inferred_animal_location != interpolated)
			continue;
		if (time == elements[k].absolute_time){
			return true;
		}
	}
	return false;
}

//generates path_aligned_images->image from region visualiation
//note that the region images contain context images (ie. the extra boundary around the region_image)
bool ns_analyzed_image_time_path::populate_images_from_region_visualization(const unsigned long time,const ns_image_standard &region_visualization,const ns_image_standard & interpolated_region_visualization,bool just_do_a_consistancy_check){
	//region visualization is all worms detected at that time point.
	ns_image_properties path_aligned_image_image_properties(region_visualization.properties());
	if (region_visualization.properties().width == 0){
		path_aligned_image_image_properties = interpolated_region_visualization.properties();
	}
	
	//	if (group_id == 42)
	//		cerr << "WHA";
	//this sets the correct image size based on the solution's information.
	//the image resolution is taken from the region visualization
	set_path_alignment_image_dimensions(path_aligned_image_image_properties);

	ns_output_subline output_subline;
	for (unsigned int k = 0; k < elements.size(); k++){
		if (time != elements[k].absolute_time) continue;
		ns_analyzed_image_time_path_element & e(elements[k]);
	//	if (group_id == 42)
	//		cerr << "WHA";
		const bool was_previously_loaded(e.path_aligned_image_is_loaded());
		try{
			if (!just_do_a_consistancy_check && !was_previously_loaded) //don't do the check for double initilaizing if we're running a consistancy check 
				e.initialize_path_aligned_images(path_aligned_image_image_properties,memory_pool->aligned_image_pool); // These allocations take a lot of time, so we pool them.  This speeds things up on machines with enough RAM to keep it all in memory.
	
			//offset_from_path is the distance the region image is from the bounding box around the path
			const ns_vector_2i tl_worm_context_position_in_pa(ns_analyzed_image_time_path::maximum_alignment_offset()+e.offset_from_path);

			if (tl_worm_context_position_in_pa.y > e.path_aligned_images->worm_region_threshold.properties().height ||
				path_aligned_image_image_properties.width > e.path_aligned_images->worm_region_threshold.properties().width || 
				tl_worm_context_position_in_pa.y > e.path_aligned_images->image.properties().height ||
				path_aligned_image_image_properties.width > e.path_aligned_images->image.properties().width)
				throw ns_ex("Out of bounds worm context position ") 
					<< path_aligned_image_image_properties.width << "," << tl_worm_context_position_in_pa.y 
					<< " for a worm with an image size " << e.path_aligned_images->worm_region_threshold.properties().width << "," << e.path_aligned_images->worm_region_threshold.properties().height;
		
			if (!just_do_a_consistancy_check){
				for (long y = 0; y < tl_worm_context_position_in_pa.y; y++){
					for (unsigned long x = 0; x < path_aligned_image_image_properties.width; x++){
						e.path_aligned_images->image[y][x] = 0;
						e.path_aligned_images->set_thresholds(y,x,0,0);
					}
				}
			}

			//fill in center
			ns_vector_2i br_worm_context_position_in_pa(tl_worm_context_position_in_pa + e.worm_context_size());
			if (br_worm_context_position_in_pa.x >= path_aligned_image_image_properties.width)
				throw ns_ex("Overflow in worm position!");
			if (br_worm_context_position_in_pa.y >= path_aligned_image_image_properties.height)
				throw ns_ex("Overflow in worm position!");

			const ns_image_standard * im(&region_visualization);
			if (e.inferred_animal_location){
				if (interpolated_region_visualization.properties().width == 0)
					throw ns_ex("No interpolated region image has been specified from which to extract inferred worm image");
				im = & interpolated_region_visualization;
				if (im->properties().width == 0)
					throw ns_ex("Required interpolated region image was not provided");
			}
			else
				if (im->properties().width == 0)
					throw ns_ex("Required region image was not provided");

			if (e.worm_context_size().x + e.context_offset_in_region_visualization_image().x > im->properties().width ||
				e.worm_context_size().y + e.context_offset_in_region_visualization_image().y > im->properties().height)
				throw ns_ex("Invalid region specification at time ") << time 
				<< "; worm position in region visualization image is larger than region visualization: The worm position: " 
				<< e.context_offset_in_region_visualization_image().x << "," << e.context_offset_in_region_visualization_image().x  
				<< "; the region image: " << im->properties().width << "," << im->properties().height << "; "
				<< " the worm context size: " << e.worm_context_size().x << "," << e.worm_context_size().y;

			if (!just_do_a_consistancy_check){
				for (long y = 0; y < e.worm_context_size().y; y++){		
					output_subline(*im,							//source
					e.context_offset_in_region_visualization_image().x,		//source x offset
					tl_worm_context_position_in_pa.x,									//dest x offset
					e.worm_context_size().x,										//width
					y+e.context_offset_in_region_visualization_image().y,	//source y offset
					y+tl_worm_context_position_in_pa.y,												//dest y offset
					e.path_aligned_images->image,1);	

					output_subline.output_specific_value(*im,							//source
					e.context_offset_in_region_visualization_image().x,		//source x offset
					tl_worm_context_position_in_pa.x,							//dest x offset
					e.worm_context_size().x,										//width
					y+e.context_offset_in_region_visualization_image().y,	//source y offset
					y+tl_worm_context_position_in_pa.y,	
					//dest y offset
					e.path_aligned_images,2,(ns_8_bit)NS_REGION_VIS_ALL_THRESHOLDED_OBJECTS_VALUE,(ns_8_bit)NS_REGION_VIS_WORM_THRESHOLD_VALUE);										//dest

					/*
					output_subline.output_specific_value(region_visualization,							//source
					e.context_offset_in_region_visualization_image().x,		//source x offset
					tl_worm_context_position_in_pa.x,											//dest x offset
					e.worm_context_size().x,										//width
					y+e.context_offset_in_region_visualization_image().y,	//source y offset
					y+tl_worm_context_position_in_pa.y,	
					//dest y offset
					e.path_aligned_images->region_threshold,2);										//dest		*/
				}
		


			//fill in gap at bottom
				for (long y = br_worm_context_position_in_pa.y; y < path_aligned_image_image_properties.height; y++){
					for (unsigned long x = 0; x < path_aligned_image_image_properties.width*path_aligned_image_image_properties.components; x++){
						e.path_aligned_images->image[y][x] = 0;
						e.path_aligned_images->set_thresholds(y,x,0,0);
			
					}

				}
			}
			
			if (just_do_a_consistancy_check && !was_previously_loaded){
				e.release_path_aligned_images(memory_pool->aligned_image_pool);
			}
		}
		catch(...){
			if (just_do_a_consistancy_check && !was_previously_loaded){
				e.release_path_aligned_images(memory_pool->aligned_image_pool);
			}
		}
		return true;
	}
	return false;
}
void ns_output_subimage(const ns_image_standard & im,const long offset,ns_image_standard & out){
	const ns_image_properties & prop(out.properties());
	const ns_image_properties & prop_i(im.properties());
	
	for (long y = 0; y < prop.height; y++)
		for (unsigned int x = 0; x < prop.width*prop.components; x++)
			out[y][x] = im[y+offset][x/prop_i.components];
}

void ns_time_path_image_movement_analyzer::add_by_hand_annotations(const ns_death_time_annotation_compiler & annotations){
	ns_death_time_annotation_compiler compiler;
	//load all paths into a compiler to do the merge
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			ns_death_time_annotation a;
			a.region_info_id = region_info_id;
			a.stationary_path_id = generate_stationary_path_id(i,j);
			compiler.add(a,ns_region_metadata());
		}
	}
	//do the merge
	compiler.add(annotations);
	//now fish all the annotations back out of the compiler, and add them to the right path
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
			ns_stationary_path_id id(generate_stationary_path_id(i,j));
			ns_death_time_annotation_compiler::ns_region_list::iterator p(compiler.regions.find(region_info_id));
			if (p == compiler.regions.end())
				throw ns_ex("ns_time_path_image_movement_analyzer::add_by_hand_annotation_event_times::Could not find region aftyer merge!");
			bool found(false);
			for (ns_death_time_annotation_compiler_region::ns_location_list::iterator q = p->second.locations.begin(); q != p->second.locations.end(); q++){
				if (q->properties.stationary_path_id == generate_stationary_path_id(i,j)){
					groups[i].paths[j].add_by_hand_annotations(*q);

					found = true;
					break;
				}
			}
			if (!found)
				throw ns_ex("ns_time_path_image_movement_analyzer::add_by_hand_annotation_event_times::Could not find path after merge!");
		}
	}
}


void ns_analyzed_image_time_path::add_by_hand_annotations(const ns_death_time_annotation_compiler_location & l){
//	if (l.properties.number_of_worms() > 1)
//		cerr << "WHEE";
	l.properties.transfer_sticky_properties(censoring_and_flag_details);
	add_by_hand_annotations(l.annotations);
}
void ns_analyzed_image_time_path::add_by_hand_annotations(const ns_death_time_annotation_set & set){
	for (unsigned int i = 0; i < set.events.size(); i++){
		const ns_death_time_annotation & e(set.events[i]);
		if (e.type != ns_translation_cessation &&
			e.type != ns_movement_cessation &&		
			e.type != ns_fast_movement_cessation &&
			e.type != ns_worm_death_posture_relaxation_termination)
			continue;

			e.transfer_sticky_properties(censoring_and_flag_details);
			if (e.type != ns_no_movement_event)
				by_hand_annotation_event_times[(int)e.type] = e.time;
	}
}

void ns_time_path_image_movement_analyzer::output_visualization(const string & base_directory) const{

	ns_dir::create_directory_recursive(base_directory);
	unsigned long p(0);

	string all_quant_filename(base_directory + DIR_CHAR_STR + "all_quant.csv");
	ofstream all_quant(all_quant_filename.c_str());
	all_quant << "path_id,time,stationary,movement,alt_movement\n";

	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
		//	cerr << "Writing out visualization for path " << p << "\n";
			if (groups[i].paths[j].element_count() == 0) continue;
			string base_dir2(base_directory + DIR_CHAR_STR + "path_" + ns_to_string(p));
			ns_dir::create_directory(base_dir2);

		//	string hist_filename(base_dir2 + DIR_CHAR_STR + "hist.csv");
		//	ofstream hist(hist_filename.c_str());
//			groups[i].paths[j].out_histograms(hist);
	//		hist.close();

			string quant_filename(base_dir2 + DIR_CHAR_STR + "quant.csv");
			ofstream quant(quant_filename.c_str());
			
			quant << "path_id,time,stationary,movement,alt_movement\n";


			string dir(base_dir2 + DIR_CHAR_STR + "raw");
			string dir_aligned(base_dir2 + DIR_CHAR_STR "registered");
			string dir_movement(base_dir2 + DIR_CHAR_STR "movement");
			
			ns_dir::create_directory_recursive(dir);
			
			ns_dir::create_directory_recursive(dir_aligned);
			ns_dir::create_directory_recursive(dir_movement);

			ns_image_standard out;	
			//ns_image_properties prop(groups[i].paths[j].element(0).path_aligned_images->image.properties());
			//prop.width = groups[i].paths[j].size.x;
			//prop.height = groups[i].paths[j].size.y;
			//out.init(prop);

			ns_image_standard out_c;

			for (unsigned int k = 0; k < groups[i].paths[j].element_count(); k++){
				quant << p << "," << groups[i].paths[j].element(k).relative_time/(60*60*24.0) << ","
					<< groups[i].paths[j].element(k).measurements.total_intensity_within_worm_area << "," 
					<< groups[i].paths[j].element(k).measurements.interframe_time_scaled_movement_sum << "," 
					<< groups[i].paths[j].elements[k].measurements.movement_alternate_worm_sum <<"\n";
				
				all_quant << p << "," << groups[i].paths[j].element(k).relative_time/(60*60*24.0) << ","
					<< groups[i].paths[j].element(k).measurements.total_intensity_within_worm_area << "," 
					<< groups[i].paths[j].element(k).measurements.interframe_time_scaled_movement_sum  << ","
					<< groups[i].paths[j].elements[k].measurements.movement_alternate_worm_sum <<"\n";

				string num;
				if (k < 10)
					num+="00";
				else if ( k < 100)
					num+="0";
				num+=ns_to_string(k);
				string filename = string(DIR_CHAR_STR) + "im_" + num + ".tif";

			//	ns_image_standard im(groups[i].paths[j].element(k).path_aligned_images->image);

			//	ns_image_properties prop(im.properties());
			//	prop.width = groups[i].paths[j].size.x;
			//	prop.height = groups[i].paths[j].size.y;
			//	out.init(prop);

			//	ns_output_subimage(im,ns_analyzed_image_time_path::maximum_alignment_offset,out);
			//	ns_save_image<ns_8_bit>(dir + filename,groups[i].paths[j].element(k).path_aligned_images->image);

			//	ns_output_subimage(im,ns_analyzed_image_time_path::maximum_alignment_offset + groups[i].paths[j].element(k).vertical_registration,out);
			//	ns_save_image(dir_aligned + filename,out);

			//	groups[i].paths[j].element(k).generate_movement_visualization(out_c);
		//		ns_save_image<ns_8_bit>(dir_movement + filename,out_c);

			}

			p++;
		}
	}
}

class ns_calc_best_alignment{
#ifndef NS_USE_INTEGER_SUMS
	typedef float ns_difference_type;
	#define ns_abs(x) fabs((x))
#else
	typedef long ns_difference_type;
	#define ns_abs(x) abs((x))

#endif
public:
	ns_calc_best_alignment(const ns_vector_2d & corse_step_,const ns_vector_2d & fine_step_, const ns_vector_2i & max_offset_, const ns_vector_2i &local_offset_,const ns_vector_2i &bottom_offset_,const ns_vector_2i &size_offset_):
		max_offset(max_offset_),local_offset(local_offset_),bottom_offset(bottom_offset_),size_offset(size_offset_),corse_step(corse_step_),fine_step(fine_step_){}

	ns_vector_2d operator()(ns_alignment_state & state,const ns_image_standard & image, bool & saturated_offset) {
		#ifdef NS_OUTPUT_ALGINMENT_DEBUG
		ofstream o("c:\\tst.txt");
		o << "step,dx,dy,sum,lowest sum\n";
		#endif
		ns_difference_type min_diff(std::numeric_limits<ns_difference_type>::max());
		ns_vector_2<float> best_offset(0,0);
		state.current_round_consensus.init(state.consensus.properties());
		for (unsigned int y = 0; y < state.consensus.properties().height; y++)
			for (unsigned int x = 0; x < state.consensus.properties().width; x++)
				state.current_round_consensus[y][x] = (state.consensus_count[y][x]!=0)?(state.consensus[y][x]/(ns_difference_type)state.consensus_count[y][x]):0;

	//	const ns_vector_2i br_s(image.properties().width-size_offset.x,image.properties().height-size_offset.y);
		
		//double area_2(((br.y-tl.y+1)/2)*((br.x-tl.x+1)/2));	
		const ns_vector_2i tl(bottom_offset),
				br((long)image.properties().width-size_offset.x,
				(long)image.properties().height-size_offset.y);
		ns_difference_type area((br.y-tl.y)*(br.x-tl.x));
		{
			ns_vector_2<float> offset_range_l(state.registration_offset_average()-local_offset),
						 offset_range_h(state.registration_offset_average()+local_offset);
			bool last_triggered_left_search(false),
				last_triggered_right_search(false),
				last_triggered_bottom_search(false),
				last_triggered_top_search(false);
			while(true){

				saturated_offset=false;
				bool left_saturated(false),top_saturated(false),right_saturated(false),bottom_saturated(false);
				
				if (offset_range_l.x-1+fine_step.x < -max_offset.x){
					offset_range_l.x = -max_offset.x+1-fine_step.x;
					saturated_offset = true;
					left_saturated = true;
				}
				if (offset_range_l.y-1+fine_step.y < -max_offset.y){
					offset_range_l.y = -max_offset.y+1-fine_step.y;
					saturated_offset = true;
					top_saturated = true;
				}
				if (offset_range_h.x+1-fine_step.x > max_offset.x){
					offset_range_h.x = max_offset.x-1+fine_step.x;
					saturated_offset = true;
					right_saturated = true;
				}
				if (offset_range_h.y+1-fine_step.y > max_offset.y){
					offset_range_h.y = max_offset.y-1+fine_step.y;
					saturated_offset = true;
					bottom_saturated = true;
				}
				//if (saturated_offset)
					//cerr << "Saturated Offset!\n";

			
				bool found_new_minimum_this_round(false);
				//very corse first search	
				const ns_vector_2i range_l(offset_range_l.x,offset_range_l.y),
								   range_h(offset_range_h.x,offset_range_h.y);
			
				for (int dy = range_l.y; dy <= range_h.y; dy++){
					for (int dx = range_l.x; dx <= range_h.x; dx++){
						ns_difference_type sum(0);
						for (long y = tl.y; y < br.y; y++){
							for (long x = tl.x; x < br.x; x++){
								sum+=ns_abs(state.current_round_consensus[y+dy][x+dx] - image[y][x]);
							}
						}
						sum/=(area);
						if (min_diff > sum){
							found_new_minimum_this_round = true;
							min_diff = sum;
							best_offset.y = dy;
							best_offset.x = dx;
						}
						#ifdef NS_OUTPUT_ALGINMENT_DEBUG
						o << "coarse," << dx << "," << dy << "," << sum << "," << min_diff << "\n";
						#endif
					}
				}
				const double nearness_to_edge_that_triggers_recalculation(2);
			//	bool l(false),r(false),t(false),b(false);
				if ( !left_saturated && found_new_minimum_this_round && (abs(best_offset.x -range_l.x) < nearness_to_edge_that_triggers_recalculation )){
					offset_range_h.x = offset_range_l.x-1;
					offset_range_l.x -= 2*local_offset.x-1;
				//	l=true;
				}
				//A previous bug here (the omission of the ! in front of !top_saturated ) caused
				//movement registration to fail in situations where images were moving up and down a lot,
				//causing animal's lifespan to be overestimated.
				else if ( !top_saturated && found_new_minimum_this_round && (abs(best_offset.y -range_l.y) < nearness_to_edge_that_triggers_recalculation )){
					offset_range_h.y = offset_range_l.y-1;
					offset_range_l.y -= 2*local_offset.y-1;
			//		t=true;
				}
				else if ( !right_saturated && found_new_minimum_this_round && (abs(best_offset.x -range_h.x) < nearness_to_edge_that_triggers_recalculation)){
					offset_range_l.x = offset_range_h.x+1;
					offset_range_h.x +=2*local_offset.x+1;
			//		r=true;
				}
				else if ( !bottom_saturated && found_new_minimum_this_round && (abs(best_offset.y -range_h.y) < nearness_to_edge_that_triggers_recalculation)){
					offset_range_l.y = offset_range_h.y+1;
					offset_range_h.y +=2*local_offset.y+1;
				//	b = true;
				}
				else 
					break;
			/*	last_triggered_left_search=l;
				last_triggered_right_search=r;
				last_triggered_bottom_search=b;
				last_triggered_top_search=t;*/
			}
		}
		#ifdef NS_DO_SUBPIXEL_REGISTRATION
		//subpixel search
		const ns_vector_2<float> center = best_offset;
		
		for (ns_difference_type dy = center.y-1+corse_step.y; dy < center.y+1; dy+=corse_step.y){
			for (ns_difference_type dx = center.x-1+corse_step.x; dx < center.x+1; dx+=corse_step.x){
				ns_difference_type sum(0);
				for (long y = tl.y; y < br.y; y++){
					for (long x = tl.x; x < br.x; x++){
						//the value of the consensus image is it's mean: consensus[y][x]/consensus_count[y][x]
						sum+=ns_abs(
							state.current_round_consensus.sample_f(y+dy,x+dx)
							/*state.consensus.weighted_sample(y+dy,x+dx,
							state.consensus_count[y + (long)dy  ][x + (long)dx ],
							state.consensus_count[y + (long)dy  ][x+ (long)dx+1],
							state.consensus_count[y + (long)dy+1][x+ (long)dx  ],
							state.consensus_count[y + (long)dy+1][x+ (long)dx+1]
							)*/
							- (ns_difference_type)image[y][x]);
					}
				}
				sum/=area;
				#ifdef NS_OUTPUT_ALGINMENT_DEBUG
				o << "fine," << dx << "," << dy << "," << sum << "," << min_diff << "\n";
				#endif
				if (min_diff > sum){
					min_diff = sum;
					best_offset.y = dy;
					best_offset.x = dx;
				}
			}
		}

		const ns_vector_2<float> center_2 = best_offset;
		for (ns_difference_type dy = center_2.y-corse_step.y+fine_step.y; dy < center_2.y+corse_step.y; dy+=fine_step.y){
			for (ns_difference_type dx = center_2.x-corse_step.x+fine_step.x; dx < center_2.x+corse_step.x; dx+=fine_step.x){
				ns_difference_type sum(0);
				for (long y = tl.y; y < br.y; y++){
					for (long x = tl.x; x < br.x; x++){	//the value of the consensus image is it's mean: consensus[y][x]/consensus_count[y][x]
						sum+=ns_abs(
							state.current_round_consensus.sample_f(y+dy,x+dx)-(ns_difference_type)image[y][x])
							;
					}
				}
				sum/=area;
				#ifdef NS_OUTPUT_ALGINMENT_DEBUG
				o << "very fine," << dx << "," << dy << "," << sum << "," << min_diff << "\n";
				#endif
				if (min_diff > sum){
					min_diff = sum;
					best_offset.y = dy;
					best_offset.x = dx;
				}
			}
		}
		#endif
		return best_offset;
	}
private:
	const ns_vector_2<float> corse_step, fine_step;
	const ns_vector_2<float> max_offset,local_offset,bottom_offset,size_offset;
	
	#undef ns_abs
};


void ns_analyzed_image_time_path_element::generate_movement_visualization(ns_image_standard & out) const{
	ns_image_properties p(registered_images->movement_image_.properties());
	p.components = 3;
	out.init(p);
	for (unsigned int y = 0; y < p.height; y++)
		for (unsigned int x = 0; x < p.width; x++){
			const unsigned short r((registered_images->movement_image_[y][x]>0)?(registered_images->movement_image_[y][x]):0);
			const unsigned short g((registered_images->movement_image_[y][x]<0)?(-registered_images->movement_image_[y][x]):0);
			//r=0;
			//if (r>255)r=255;
			//r = (r+registered_images->image[y][x]);
			//if (r >= 255)r=255;
			out[y][3*x] = r;
			out[y][3*x+1] = g;
			out[y][3*x+2] = registered_images->image[y][x];
		}
}
class ns_debug_image_out{
public:
	void operator ()(const string &dir, const string & filename, const unsigned long kernal_size,const ns_image_whole<double> & im, const ns_image_whole<ns_16_bit> & consensus_count, const ns_image_whole<ns_8_bit> & unaligned_image){
		string filename_im(dir + DIR_CHAR_STR + "consensus" + DIR_CHAR_STR + filename),
			   filename_count(dir + DIR_CHAR_STR + "count" + DIR_CHAR_STR + filename),
			   filename_unaligned(dir + DIR_CHAR_STR + "unaligned" + DIR_CHAR_STR + filename);
		ns_dir::create_directory_recursive(dir + DIR_CHAR_STR + "consensus");
		ns_dir::create_directory_recursive(dir + DIR_CHAR_STR + "count");
		ns_dir::create_directory_recursive(dir + DIR_CHAR_STR + "unaligned");
		ns_image_properties prop(im.properties());
		count_swap.init(prop);
		im_swap.init(prop);

		for (unsigned int y = 0; y < prop.height; y++){
			for (unsigned int x = 0; x < prop.width; x++){
				im_swap[y][x] = (ns_8_bit)(im[y][x]/consensus_count[y][x]);
				count_swap[y][x] = (ns_8_bit)((((unsigned long)255)*consensus_count[y][x])/(kernal_size));
			}
		}
		ns_save_image<ns_8_bit>(filename_im,im_swap);
		ns_save_image(filename_count,count_swap);
		ns_save_image(filename_unaligned,unaligned_image);
	}
private:
	ns_image_whole<ns_8_bit> count_swap;
	ns_image_whole<ns_8_bit> im_swap;
};
//possible range of a sobel operator: (0,255)
inline ns_8_bit ns_sobel(const ns_image_standard & im,const unsigned long &x, const unsigned long & y){
	const unsigned long c(ns_analyzed_image_time_path::sobel_operator_width);
	const long sobel_y((long)im[y-c][x-c]+2*(long)im[y-c][x]+(long)im[y-c][x+c]
					  -(long)im[y+c][x-c]-2*(long)im[y+c][x]-(long)im[y+c][x+c]);
	const long sobel_x((long)im[y-c][x-c]+2*(long)im[y][x-c]+(long)im[y+c][x-c]
					  -(long)im[y-c][x+c]-2*(long)im[y][x+c]-(long)im[y+c][x+c]);
	return (ns_8_bit)(sqrt((double)(sobel_x*sobel_x+sobel_y*sobel_y))/5.65685);
}

class ns_movement_data_accessor{
public:
	ns_movement_data_accessor(ns_analyzed_image_time_path::ns_element_list & l):elements(&l){}
	unsigned long size()const {return elements->size();}
	const unsigned long time(const unsigned long i){return (*elements)[i].absolute_time;}
	double raw(const unsigned long i){
		if (i == 0)
			return raw(1);
		if ((*elements)[i-1].measurements.total_intensity_within_worm_area == 0)
			return 0;
		//XXX This is the definition of a "movement score".
		return  (
					(*elements)[i].measurements.unnormalized_movement_sum-
					//this term normalizes for changes in absolute intensity between frames (ie. light levels changing or worm size changes)
					fabs((double)(*elements)[i].measurements.total_intensity_within_worm_area 
					- (double)(*elements)[i-1].measurements.total_intensity_within_worm_area)
					
				)
				/
				(*elements)[i-1].measurements.total_intensity_within_worm_area;
		
	}
	double & processed(const unsigned long i){
		return (*elements)[i].measurements.death_time_posture_analysis_measure();
	}
private:
	ns_analyzed_image_time_path::ns_element_list * elements;
};

class ns_intensity_data_accessor{
public:
	ns_intensity_data_accessor(ns_analyzed_image_time_path::ns_element_list & l):elements(&l){}
	unsigned long size()const {return elements->size();}
	const unsigned long time(const unsigned long i){return (*elements)[i].absolute_time;}
	double raw(const unsigned long i){
		return (*elements)[i].measurements.change_in_average_normalized_worm_intensity;
	}
	double & processed(const unsigned long i){
		return (*elements)[i].measurements.denoised_change_in_average_normalized_worm_intensity;
	}
private:
	ns_analyzed_image_time_path::ns_element_list * elements;
};

class ns_size_data_accessor{
public:
	ns_size_data_accessor(ns_analyzed_image_time_path::ns_element_list & l):elements(&l){}
	unsigned long size()const {return elements->size();}
	const unsigned long time(const unsigned long i){return (*elements)[i].absolute_time;}
	double raw(const unsigned long i){
		return (*elements)[i].measurements.total_worm_area;
	}
	double & processed(const unsigned long i){
		return (*elements)[i].measurements.normalized_worm_area;
	}
private:
	ns_analyzed_image_time_path::ns_element_list * elements;
};

template <class T>
class ns_kernel_smoother{
public:
	void ns_kernel_smooth(const unsigned int kernel_width,T & data){
		(*this)(kernel_width,data);
	}
	void operator()(const int kernel_width,T & data){
		if (data.size() == 0)
			return;
		if (kernel_width%2 == 0)
			throw ns_ex("Kernel width must be odd");
		
		if (1){
			for (unsigned int i = 0; i < kernel_width; i++)
				data.processed(i) = data.raw(i);
			for (unsigned int i = kernel_width; i < data.size()-kernel_width; i++){
				double s(0);
				for (int di = -kernel_width; di <= kernel_width; di++)
					s+= data.raw(i+di);
				s+=data.raw(i);
				data.processed(i)=s/(2*kernel_width+2);
			}
			for (unsigned int i = data.size()-kernel_width; i < data.size(); i++)
				data.processed(i) = data.raw(i);
		}
	}
};
void ns_analyzed_image_time_path::denoise_movement_series( const ns_time_series_denoising_parameters & times_series_denoising_parameters){
	if (elements.size() == 0)
		return;

	const bool use_kernal_smoother(true);
	if (use_kernal_smoother){
		
		const int kernel_width(1);
		ns_kernel_smoother<ns_movement_data_accessor>m;
		ns_movement_data_accessor acc(elements);
		
		//Here, ns_movement_data_accessor calculates the movement score and stores it.
		//This movement score is generally not used for automated movement analysis, but is useful for understanding worm behavior and debugging.
		for (unsigned int i = 0; i < elements.size(); i++){
			elements[i].measurements.movement_score = acc.raw(i);
			if (i > 0)
				elements[i].measurements.change_in_total_worm_intensity =  (double)elements[i].measurements.total_intensity_within_worm_area - (double)elements[i-1].measurements.total_intensity_within_worm_area;
			else elements[i].measurements.change_in_total_worm_intensity = 0;
		}

		//Here, ns_movement_data_accessor calculates and provides the raw movement score to the kernel smoother,
		//which then uses it to calculate the denoised movement scores
		//These denoised movmenet scores are used for automated movement analaysis.
		//If the "normalize_movement_timeseries_to_median" flag is set, the set smoothed movement score values
		//for each object is subtracted out to zero.
		m(kernel_width,acc);
		
		for (unsigned int i= 0; i < elements.size(); i++){
			elements[i].measurements.normalized_worm_area = elements[i].measurements.total_worm_area/(double)elements[0].measurements.total_worm_area;
			elements[i].measurements.normalized_total_intensity = elements[i].measurements.total_intensity_within_worm_area/(double)elements[0].measurements.total_intensity_within_worm_area;
			if (i > 0)
				elements[i].measurements.change_in_average_normalized_worm_intensity = elements[i].measurements.normalized_total_intensity - elements[i-1].measurements.normalized_total_intensity;
			else elements[i].measurements.change_in_average_normalized_worm_intensity = 0;
		}
		if (elements.size() == 1)
			elements[0].measurements.change_in_average_normalized_worm_intensity = 0;
		else elements[0].measurements.change_in_average_normalized_worm_intensity = elements[1].measurements.change_in_average_normalized_worm_intensity;
		
		//ns_kernel_smoother<ns_intensity_data_accessor> i;
		//i(kernel_width,ns_kernel_smoother<ns_intensity_data_accessor>::ns_fix_to_first_value,ns_intensity_data_accessor(elements));
		//ns_kernel_smoother<ns_size_data_accessor> s;
		//s(kernel_width,ns_kernel_smoother<ns_size_data_accessor>::ns_fix_to_first_value,ns_size_data_accessor(elements));
		
	/*	for (unsigned int i = 1; i < elements.size();i++)
			elements[i].measurements.normalized_change_in_worm_intensity = 
			elements[i].measurements.normalized_total_intensity - 
			elements[i-1].measurements.normalized_total_intensity;
		if (elements.size() == 1)
		elements[0].measurements.normalized_change_in_worm_intensity = 0;
		else elements[0].measurements.normalized_change_in_worm_intensity =
			elements[1].measurements.normalized_change_in_worm_intensity;*/
		ns_kernel_smoother<ns_intensity_data_accessor> i;
		ns_intensity_data_accessor acc2(elements);
		i(kernel_width,acc2);
		
		return;
	}
	else{
		const int offset_size(4);
		const bool use_median(true);
		std::vector<double> normalization_factors(offset_size);
		if (use_median){
			std::vector<std::vector<unsigned long> > values(offset_size);
			for (unsigned int i=0; i < offset_size; i++){
				values[i].reserve(elements.size()/offset_size);
			}
			int k(0);
			for (unsigned int i = 0; i < elements.size(); i++){
				values[k].push_back(elements[i].measurements.unnormalized_movement_sum);
				k++;
				if (k==offset_size) k = 0;
			}
			for (unsigned int i=0; i < offset_size; i++){
				std::sort(values[i].begin(),values[i].end());
				normalization_factors[i] = values[i][values[i].size()/2];
			}
		}
		else{
			long long sums[offset_size];
			long count[offset_size];
			//initialize
			for (unsigned int i = 0; i < offset_size; i++){
				sums[i] = 0;
				count[i] = 0;
			}
			//calculate mean of each offset, first by calculating sum and count
			int k(0);
			for (unsigned int i = elements.size()/3; i < elements.size(); i++){
				sums[k] += elements[i].measurements.unnormalized_movement_sum;
				count[k]++;
				k++;
				if (k==offset_size) k = 0;
			}
			//then dividing to get the means.
			for (unsigned int i = 0; i < offset_size; i++)
				normalization_factors[i] = sums[i]/(double)count[i];
		}
		//now normalize each movement score by its offset's mean
		int k = 0;
		for (unsigned int i = 0; i < elements.size(); i++){
			elements[i].measurements.denoised_movement_score = elements[i].measurements.unnormalized_movement_sum/(double)normalization_factors[k];
			k++;
			if (k==offset_size) k = 0;
		}
	}
}
void ns_analyzed_image_time_path::quantify_movement(const ns_analyzed_time_image_chunk & chunk){
	
	/*for (unsigned long i = 0; i < 256; i++){
		movement_histogram[i]=0;
		stationary_histogram[i]=0;
	}*/
	//get the image size for temporary images.  
	ns_image_properties prop;;
	bool found_one_not_excluded(false);
	for (unsigned int i = chunk.start_i; i < chunk.stop_i; i++){
		elements[i].measurements.zero();
		if (!elements[i].excluded){
			if (elements[chunk.start_i].registered_images == 0)
				throw ns_ex("Unallocated registered image encountered: ") << i << " with 0 position as " << this->first_stationary_timepoint();
			prop = elements[chunk.start_i].registered_images->image.properties();
			found_one_not_excluded = true;
		}
	}
	if (!found_one_not_excluded)
		return;

	const int image_width(prop.width),
			  image_height(prop.height);
	if (image_analysis_temp1 == 0){
		image_analysis_temp1 = new unsigned char[image_width*image_height];
		image_analysis_temp2  = new unsigned char[image_width*image_height];
	}

	for (unsigned int i = chunk.start_i; i < chunk.stop_i; i++){
		elements[i].measurements.zero();

		if (elements[i].excluded) continue;
	//	elements[i].measurements.registration_offset = elements[i].registration_offset.mag();
		if (elements[i].registered_images == 0)
			throw ns_ex("ns_analyzed_image_time_path::quantify_movement()::Encountered an unloaded registered image!");
		for (unsigned long y = 0; y < elements[i].registered_images->movement_image_.properties().height; y++){
			for (unsigned long x = 0; x < elements[i].registered_images->movement_image_.properties().width; x++){
				const bool worm_threshold(elements[i].registered_images->get_worm_neighborhood_threshold(y,x));
				elements[i].measurements.total_intensity_within_region+=elements[i].registered_images->image[y][x];
				elements[i].measurements.total_intensity_within_worm_area+=worm_threshold?elements[i].registered_images->image[y][x]:0;
				elements[i].measurements.total_worm_area += (worm_threshold?1:0);
			}
		}
		
	//	elements[i].measurements.total_region_area=elements[i].registered_images->movement_image_.properties().height*elements[i].registered_images->movement_image_.properties().width;
	//	double interframe_scaling_factor;
		if (i < movement_time_kernel_width){
				elements[i].measurements.change_in_average_normalized_worm_intensity = 
				elements[i].measurements.change_in_average_region_intensity = 0;
	//			interframe_scaling_factor = 1;
		}
		else{
			elements[i].measurements.change_in_average_region_intensity = 
				elements[i].measurements.total_intensity_within_region/(double)elements[i].measurements.total_region_area 
				-elements[i-movement_time_kernel_width].measurements.total_intensity_within_region/(double)elements[i-movement_time_kernel_width].measurements.total_region_area;
			
			if (elements[i].measurements.total_worm_area  == 0 || elements[i-movement_time_kernel_width].measurements.total_worm_area==0 ){
				elements[i].measurements.change_in_average_normalized_worm_intensity = 0;
		//		interframe_scaling_factor = 1;
			}
			else{
				elements[i].measurements.change_in_average_normalized_worm_intensity = 
					elements[i].measurements.total_intensity_within_worm_area/(double)elements[i].measurements.total_worm_area 
					-elements[i-movement_time_kernel_width].measurements.total_intensity_within_worm_area/(double)elements[i-movement_time_kernel_width].measurements.total_worm_area;
	//			if (elements[i].measurements.total_intensity_within_worm_area == 0)
	//				interframe_scaling_factor = 1;
	//			else 
		//			interframe_scaling_factor = (elements[i-movement_time_kernel_width].measurements.total_intensity_within_region)
		//										/ ((double)elements[i].measurements.total_intensity_within_region);
			}
		}
		double movement_sum(0),
			   alternate_movement_sum(0);
		elements[i].measurements.unnormalized_movement_sum = 0;
		for (unsigned long y = 0; y < elements[i].registered_images->movement_image_.properties().height; y++){
			for (unsigned long x = 0; x < elements[i].registered_images->movement_image_.properties().width; x++){
				
		//		movement_histogram[elements[i].registered_images->movement_image_[y][x]]++;
		//		stationary_histogram[elements[i].registered_images->image[y][x]]++;
				const bool worm_threshold(elements[i].registered_images->get_worm_neighborhood_threshold(y,x));

				const bool alternate_worm_threshold(elements[i].registered_images->get_region_threshold(y,x) == 1 && 
													!worm_threshold);
				elements[i].measurements.unnormalized_movement_sum+=(worm_threshold?abs(elements[i].registered_images->movement_image_[y][x]):0);

				/*const double m(
					fabs(
					elements[i].registered_images->movement_image_[y][x]+
					(elements[i].registered_images->image[y][x]*(interframe_scaling_factor-1))
					)
				);
				movement_sum += worm_threshold?m:0.0;*/
				image_analysis_temp1[y*image_width+x] = worm_threshold?elements[i].registered_images->movement_image_[y][x]:0;
				
				alternate_movement_sum+= (alternate_worm_threshold)?elements[i].registered_images->movement_image_[y][x]:0;

				elements[i].measurements.total_alternate_worm_area += (alternate_worm_threshold?1:0);
				elements[i].measurements.total_intensity_within_alternate_worm += alternate_worm_threshold?elements[i].registered_images->image[y][x]:0;

			}
		}
		double interframe_time_scaling_factor(1);
		if (i >= movement_time_kernel_width){
			const long dt_s(elements[i].absolute_time - elements[i-movement_time_kernel_width].absolute_time);
			const double standard_interval(60*60);//one hour
			interframe_time_scaling_factor = standard_interval/dt_s;
		}
		
		elements[i].measurements.interframe_time_scaled_movement_sum=elements[i].measurements.unnormalized_movement_sum*interframe_time_scaling_factor;
		elements[i].measurements.movement_alternate_worm_sum= (unsigned long)alternate_movement_sum;

		const int kernel_half_width(11);
		if (image_width < 2*kernel_half_width || image_height < 2*kernel_half_width){
			elements[i].measurements.local_maximum_movement_sum = (unsigned long)elements[i].measurements.interframe_time_scaled_movement_sum*interframe_time_scaling_factor;
			elements[i].measurements.local_maximum_stationary_sum = elements[i].measurements.total_intensity_within_worm_area;
		}
		else{
			//treats edges as zero padded
			ctmf_orig(image_analysis_temp1,image_analysis_temp2,image_width,image_height,
				 image_width,image_width,
				 kernel_half_width,1,512*1024);
			#ifdef NS_OUTPUT_ALGINMENT_DEBUG
			ns_image_standard im1;
			im1.prepare_to_recieve_image(elements[i].registered_images->image.properties());
			for (unsigned long y = 0; y < elements[i].registered_images->movement_image_.properties().height; y++){
				for (unsigned long x = 0; x < elements[i].registered_images->movement_image_.properties().width; x++){
					im1[y][x] = image_analysis_temp1[image_width*y+x];
				}
			}
			ns_save_image(std::string("c:\\movement\\registration\\im_a") + ns_to_string(i) + ".tif",im1);

			for (unsigned long y = 0; y < elements[i].registered_images->movement_image_.properties().height; y++){
				for (unsigned long x = 0; x < elements[i].registered_images->movement_image_.properties().width; x++){
					im1[y][x] = image_analysis_temp2[image_width*y+x];
				}
			}
			ns_save_image(std::string("c:\\movement\\registration\\im_b") + ns_to_string(i) + ".tif",im1);

			#endif
			unsigned long max_m(0),
				max_y(kernel_half_width),max_x(kernel_half_width);
			for (unsigned int y = kernel_half_width; y < image_height-kernel_half_width; y++){
				for (unsigned int x = kernel_half_width; x < image_width-kernel_half_width; x++){
					if (max_m < image_analysis_temp2[image_width*y+x]){
						max_m = image_analysis_temp2[image_width*y+x];
						max_y = y;
						max_x = x;
					}
				}
			}
			elements[i].measurements.local_maximum_position = ns_vector_2i(max_x-kernel_half_width,max_y-kernel_half_width);
			elements[i].measurements.local_maximum_area = ns_vector_2i(2*kernel_half_width+1,2*kernel_half_width+1);
			elements[i].measurements.local_maximum_movement_sum = 0;
			elements[i].measurements.local_maximum_stationary_sum = 0;
			double local_movement_sum(0);
			for (int dy = -kernel_half_width; dy <= kernel_half_width; dy++){
				for (int dx = -kernel_half_width; dx <= kernel_half_width; dx++){
					const int y(max_y+dy),
							 x(max_x+dx);
					const bool worm_threshold(elements[i].registered_images->get_worm_neighborhood_threshold(y,x));
				/*	const double m(
						fabs(
						elements[i].registered_images->movement_image_[y][x]+
						(elements[i].registered_images->image[y][x]*(interframe_scaling_factor-1))
						)
					);*/
					local_movement_sum+=worm_threshold?abs(elements[i].registered_images->movement_image_[y][x]):0.0;
					elements[i].measurements.local_maximum_stationary_sum+=worm_threshold?elements[i].registered_images->image[y][x]:0;
				}
			}
			elements[i].measurements.local_maximum_movement_sum = (unsigned long)(local_movement_sum*interframe_time_scaling_factor);
		}
	}
}

void ns_analyzed_image_time_path::generate_movement_images(const ns_analyzed_time_image_chunk & chunk){
	const long n(movement_detection_kernal_half_width);
	const long kernel_area((2*n+1)*(2*n+1));

	const unsigned long C(256);
	const unsigned long C_sqrt(16);

	ns_image_properties prop(elements[chunk.start_i].path_aligned_images->image.properties());
	set_path_alignment_image_dimensions(prop);
	if (!(prop == elements[chunk.start_i].path_aligned_images->image.properties()))
		throw ns_ex("The path aligned image has the wrong size!");

	registered_image_properties = prop;
	prop.components = 1;
	float m_min(FLT_MAX),m_max(0);

	memory_pool->set_temporary_image_size(3);
	ns_image_standard & worm_threshold(memory_pool->temporary_images[0]),
					& region_threshold(memory_pool->temporary_images[1]),
					& worm_neighborhood_threshold(memory_pool->temporary_images[2]);
	worm_threshold.prepare_to_recieve_image(prop);
	region_threshold.prepare_to_recieve_image(prop);
	worm_neighborhood_threshold.prepare_to_recieve_image(prop);

	long step(chunk.forward()?1:-1);
	const long dt(movement_time_kernel_width);
	const unsigned long movement_start_i(chunk.forward()?((chunk.start_i>dt)?chunk.start_i:dt)
														: ((chunk.start_i-dt)));

	for (long i = chunk.start_i; ; i+=step){
		if (chunk.forward() && i >= chunk.stop_i)
			break;
		if (!chunk.forward() && i <= chunk.stop_i)
			break;

		if (elements[i].excluded){
	//		cerr << "Skipping Excluded\n";
			continue;
		}
		elements[i].element_was_processed = true;
//		cerr << "Initializing registered images for " << i << "\n";
		
		if (!chunk.forward() || !elements[i].registered_image_is_loaded() || (i > ns_analyzed_image_time_path::alignment_time_kernel_width && i > this->first_stationary_timepoint()))
			elements[i].initialize_registered_images(prop,memory_pool->registered_image_pool);

		
		//fill in top border

		//mapping from pixels in im to pixels in im_1


		//ok the tricky thing here is that each individual image is valid between [0, elements[i].worm_context_size()]
		//but the *intersection* between two consecutive frames, which we want to quantify
		//is only valid within the overlapping range [0 + offset, elements[i].worm_context_size+offset].
		//For each image we output the entire [0,element[i].worm_context_size()] offset by v0
		//to registered_images->image, registered_images->region_threshold, and registered_images->worm_threshold
		//but for the registered_movement_image we only output the minimal overlap which is
		// [maximum_alignment_offset()
		const ns_vector_2<float> v0(elements[i].registration_offset);

		ns_vector_2i tl(maximum_alignment_offset()+elements[i].offset_from_path);
		ns_vector_2i br(tl+elements[i].worm_context_size());
		if (br.x > prop.width-maximum_alignment_offset().x)
			br.x = prop.width-maximum_alignment_offset().x;
		if (br.y > prop.width-maximum_alignment_offset().y)
			br.y = prop.width-maximum_alignment_offset().y;

		//here we crop 2i
		//so the registered image rage may be slightly smaller than it could be
		//as we miss the left or right fraction of a pixel
		ns_vector_2i tl_registered(maximum_alignment_offset()+ns_vector_2i(v0.x,v0.y)+elements[i].offset_from_path);
		ns_vector_2i br_registered(tl_registered+elements[i].worm_context_size());
		//ns_vector_2i br(ns_vector_2i(elements[0].path_aligned_images->image.properties().width,elements[0].path_aligned_images->image.properties().height)-maximum_alignment_offset());

		//fill bottom
		for (long y = 0; y < tl_registered.y; y++){
			for (long x = 0; x < prop.width; x++){
				elements[i].registered_images->image			[y][x] = NS_MARGIN_BACKGROUND;

				region_threshold	[y][x] = NS_MARGIN_BACKGROUND;
				worm_threshold	[y][x] = NS_MARGIN_BACKGROUND;
			}
		}	
		
		for (long y = tl_registered.y; y < br_registered.y; y++){
			//fill in left gap
			for (long x = 0; x < (tl_registered.x); x++){
				elements[i].registered_images->image			[y][x] = NS_MARGIN_BACKGROUND;
				region_threshold	[y][x] = NS_MARGIN_BACKGROUND;
				worm_threshold	[y][x] = NS_MARGIN_BACKGROUND;
			}
			//fill in center
			for (long x = tl_registered.x; x < br_registered.x; x++){
				elements[i].registered_images->image		   [y][x] =		       elements[i].path_aligned_images->image			.sample_f(y-v0.y,x-v0.x);
				elements[i].path_aligned_images->sample_thresholds<ns_8_bit>(y-v0.y,x-v0.x,region_threshold[y][x],worm_threshold[y][x]);
			}
			//fill in right gap
			for (long x = (br_registered.x); x < prop.width; x++){
				elements[i].registered_images->image[y][x] = NS_MARGIN_BACKGROUND;
				region_threshold[y][x] = NS_MARGIN_BACKGROUND;
				worm_threshold[y][x] = NS_MARGIN_BACKGROUND;
			}
		}
		//fill in top
		for (long y = br_registered.y; y < prop.height; y++){
			for (long x = 0; x < prop.width; x++){
				elements[i].registered_images->image[y][x] = NS_MARGIN_BACKGROUND;
				worm_threshold[y][x] = NS_MARGIN_BACKGROUND;
				region_threshold[y][x] = NS_MARGIN_BACKGROUND;
			}
		}
		//now we generate the movement image
		bool first_frames(chunk.forward()?(i < movement_start_i):(i > movement_start_i));
		const ns_vector_2d v1(first_frames?ns_vector_2d(0,0):elements[i-step*dt].registration_offset);
		//fill in bottom
		for (long y = 0; y < tl.y+n; y++){
			for (long x = 0; x < prop.width; x++){
				elements[i].registered_images->movement_image_	[y][x] = NS_MARGIN_BACKGROUND;
			}
		}	
		for (long y = tl.y+n; y < br.y-n; y++){
			//fill in left gap
			for (long x = 0; x < (tl.x+n); x++)
				elements[i].registered_images->movement_image_	[y][x] = NS_MARGIN_BACKGROUND;
			
			//fill in center
			for (long x = tl.x+n; x < br.x-n; x++){
				
				#ifdef NS_DEBUG_IMAGE_ACCESS
				if (y >= elements[i].registered_images->image.properties().height ||
					x >= elements[i].registered_images->image.properties().width)
					throw ns_ex("Yikes");
				#endif

				float d_numerator(0),
						  d_denominator(0);
				//if we're at the beginning there is no previous frames to use, so we just assume all pixels are moving
				//and output the worm image as the movement image
				if (first_frames){
					for (long dy=-n; dy <= n; dy++){
						for (long dx=-n; dx <= n; dx++){
					//		const long grad(ns_sobel(im,x+dx,y+dy+vr0));
					//		d_numerator+=labs((long)im_1[y+dy+vr1][x+dx]-(long)im[y+dy+vr0][x+dx])*grad;
					//		d_denominator+=grad*grad;
							float d(elements[i].   path_aligned_images->image.sample_f(y-(v0.y) + dy,
												x-(v0.x) + dx));
						//	d*=elements[i].path_aligned_images->worm_threshold.sample_f(y-(v0.y) + dy,
							//					x-(v0.x) + dx);
						///OLD STYLE	d_numerator+=(d>0?d:0);
							d_numerator+=d;
						}
					}
				}
				else{
					for (long dy=-n; dy <= n; dy++){
						for (long dx=-n; dx <= n; dx++){
					//		const long grad(ns_sobel(im,x+dx,y+dy+vr0));
					//		d_numerator+=labs((long)im_1[y+dy+vr1][x+dx]-(long)im[y+dy+vr0][x+dx])*grad;
					//		d_denominator+=grad*grad;
							float d(elements[i].   path_aligned_images->image.sample_f(y-(v0.y) + dy,
												x-(v0.x) + dx) 
												-
									 elements[i-step*dt].path_aligned_images->image.sample_f(y-(v1.y) + dy,
												x-(v1.x) + dx));  //subtract vr0 from both because y is in global reference frame and for im_1 we've already added vr1 so the result is vr1-vr0 which gives us the desired offset.
						///OLD_STYLE	d_numerator+=(d>0?d:0);
							d_numerator+=d;
						}
					}
				}
				
				float val = d_numerator/=kernel_area;
				if (val < m_min)
					m_min = val;
				if (val > m_max)
					m_max = val;
				///OLD_STYLE	if (val > 255) val = 255;
				//note that the absolute value of val can never be greater than 255  
				//so val contains 9 bits of information

				elements[i].registered_images->movement_image_[y][x] = (short)val;

			}
			//fill in right gap
			for (long x = (br.x-n); x < prop.width; x++){
				elements[i].registered_images->movement_image_[y][x] = NS_MARGIN_BACKGROUND;
			}
		}
		//fill in top
		for (long y = br.y-n; y < prop.height; y++){
			for (long x = 0; x < prop.width; x++){
				elements[i].registered_images->movement_image_[y][x] = NS_MARGIN_BACKGROUND;
			}
		}
		ns_dilate<16,ns_image_standard,ns_image_standard>(worm_threshold,worm_neighborhood_threshold);
		for (unsigned int y = 0; y < worm_threshold.properties().height; y++)
			for (unsigned int x = 0; x < worm_threshold.properties().width; x++)
				elements[i].registered_images->set_thresholds(y,x,region_threshold[y][x],worm_threshold[y][x],worm_neighborhood_threshold[y][x]);
	}

	
	//
	//for (unsigned int i = chunk.start_i; i < movement_start_i; i++){
	//	elements[start_i].registered_movement_image.pump(elements[i].registered_movement_image,1024);
		//elements[start_i].registered_images->image.pump(elements[i].registered_images->image,1024);
		//elements[start_i].registered_images->region_threshold.pump(elements[i].registered_images->region_threshold,1024);
		//elements[start_i].registered_images->worm_threshold.pump(elements[i].registered_images->worm_threshold,1024);
	//}
/*	for (unsigned int i = chunk.start_i; i < chunk.stop_i; i++){
		string filename = "y:\\debug\\im_";
		filename += ns_to_string(i);
		ns_save_image(filename + "path.tif",elements[i].path_aligned_images->image);
		ns_save_image(filename + "reg.tif",elements[i].registered_images->image);
		ns_save_image(filename + "mov.tif",elements[i].registered_movement_image);

	}*/
}
ns_analyzed_time_image_chunk ns_analyzed_image_time_path::initiate_image_registration(const ns_analyzed_time_image_chunk & chunk,ns_alignment_state & state){
	const unsigned long time_kernel(alignment_time_kernel_width);
	if (abs(chunk.stop_i - chunk.start_i) < time_kernel)
		throw ns_ex("ns_analyzed_image_time_path::initiate_image_registration()::First chunk must be >= time kernel width!");
	ns_analyzed_time_image_chunk remaining(chunk);
	if (chunk.forward())
	remaining.start_i += time_kernel;
	else remaining.start_i -=time_kernel;

	const unsigned long first_index(chunk.start_i);
	ns_image_properties prop(elements[first_index].path_aligned_images->image.properties());
	set_path_alignment_image_dimensions(prop);

	state.consensus.init(prop);
	state.consensus_count.init(prop);

	//image is offset by maximum_alignment offset in each direction
	//but we the largest vertical alignment in either direction is actually half that,
	//because two consecutive frames can be off by -max and max, producing a overal differential of 2*max offset between the two images


	//if (max_alignment_offset.x < 0) max_alignment_offset.x = 0;
	//if  (max_alignment_offset.y < 0) max_alignment_offset.y = 0;

	const ns_vector_2i bottom_offset(maximum_alignment_offset());
	const ns_vector_2i top_offset(maximum_alignment_offset());

	ns_calc_best_alignment align(NS_SUBPIXEL_REGISTRATION_CORSE,NS_SUBPIXEL_REGISTRATION_FINE,maximum_alignment_offset(),maximum_local_alignment_offset(),bottom_offset,top_offset);
	
	//load first consensus kernal
	
	const ns_vector_2i d(maximum_alignment_offset());	
	const ns_vector_2i h_sub(ns_vector_2i(state.consensus.properties().width,
										  state.consensus.properties().height)
										  -d);
	for (unsigned long y = 0; y < prop.height; y++){
		for (unsigned long x = 0; x < prop.width; x++){
			state.consensus[y][x] = 0;
			state.consensus_count[y][x] = 0;
		}
	}
	for (long y = d.y; y < h_sub.y; y++){
			for (long x = d.x; x < h_sub.x; x++){
			state.consensus[y][x] = elements[first_index].path_aligned_images->image[y][x];
			state.consensus_count[y][x] = 1;
		}
	}
	state.registration_offset_sum = ns_vector_2i(0,0);
	state.registration_offset_count=1;

	#ifdef NS_OUTPUT_ALGINMENT_DEBUG
	ns_debug_image_out out;
	out("c:\\movement",debug_path_name + "0.tif",alignment_time_kernel_width,state.consensus,state.consensus_count,elements[0].path_aligned_images->image);
	#endif
	elements[first_index].registration_offset = ns_vector_2i(0,0);
	//ns_vector_2i test_alignment = align(state,elements[0].path_aligned_images->image);
	//cerr << "TAlignment: " << test_alignment << "\n";
	for (long r_i = 1; r_i < time_kernel; r_i++){
		long i(chunk.forward()?(first_index+r_i):(first_index-r_i));
		if ( elements[i].path_aligned_images->image.properties().height == 0)
			throw ns_ex("ns_analyzed_image_time_path::calculate_vertical_registration()::Element ")  << i << " was not assigned!";
		
		elements[i].registration_offset = align(state,elements[i].path_aligned_images->image,elements[i].saturated_offset);
	//	cerr << "Alignment: " << elements[i].registration_offset << "\n";
		
		state.registration_offset_sum += elements[i].registration_offset;
		state.registration_offset_count++;

	//	cerr << "O:" << elements[i].vertical_registration << "\n";
		//cerr << i << ":" << (unsigned long)consensus[10][21] << "<-" << (unsigned long)elements[i].path_aligned_images->image[10-maximum_alignment_offset()-elements[i].vertical_registration][21] << "\n";
		for (long y = d.y; y < h_sub.y; y++){
			for (long x = d.x; x < h_sub.x; x++){
				state.consensus[y][x] += elements[i].path_aligned_images->image.sample_f(y-elements[i].registration_offset.y,
																						 x-elements[i].registration_offset.x);
				state.consensus_count[y][x]++;
			}
		}
		#ifdef NS_OUTPUT_ALGINMENT_DEBUG
		out("c:\\movement",debug_path_name + ns_to_string(i) + ".tif",alignment_time_kernel_width,state.consensus,state.consensus_count,elements[i].path_aligned_images->image);
		#endif
		} 

	return remaining;
}

void ns_analyzed_image_time_path::calculate_image_registration(const ns_analyzed_time_image_chunk & chunk_,ns_alignment_state & state,const ns_analyzed_time_image_chunk & first_chunk){
	ns_analyzed_time_image_chunk chunk(chunk_);
	//if the registration needs to be initialized, do so.
	if (chunk.start_i == first_chunk.start_i)
		chunk = initiate_image_registration(chunk_,state);
	

	//make alignment object
		//image is offset by maximum_alignment offset in each direction
	//but we the largest vertical alignment in either direction is actually half that,
	//because two consecutive frames can be off by -max and max, producing a overal differential of 2*max offset between the two images

	ns_vector_2i max_alignment_offset(maximum_alignment_offset().x/2-movement_detection_kernal_half_width,
											maximum_alignment_offset().y/2-movement_detection_kernal_half_width);
	if (max_alignment_offset.x < 0) max_alignment_offset.x = 0;
	if (max_alignment_offset.y < 0) max_alignment_offset.y = 0;

	const ns_vector_2i bottom_offset(maximum_alignment_offset());
	const ns_vector_2i top_offset(maximum_alignment_offset());
	
	ns_calc_best_alignment align(NS_SUBPIXEL_REGISTRATION_CORSE,NS_SUBPIXEL_REGISTRATION_FINE,maximum_alignment_offset(),maximum_local_alignment_offset(),bottom_offset,top_offset);

	const long time_kernal(alignment_time_kernel_width);

	//define some constants
	ns_image_properties prop;
	set_path_alignment_image_dimensions(prop);
	ns_vector_2i d(maximum_alignment_offset());	
	const ns_vector_2i h_sub(ns_vector_2i(state.consensus.properties().width,
										  state.consensus.properties().height)
										  -d);
	long step(chunk.forward()?1:-1);
	for (long i = chunk.start_i; ; i+=step){
		if (chunk.forward() && i >= chunk.stop_i)
			break;
		if (!chunk.forward() && i <= chunk.stop_i)
			break;

		if (elements[i].excluded) continue;

		if (!elements[i-step*time_kernal].path_aligned_image_is_loaded()){
			throw ns_ex("Image for ") << i-time_kernal << " isn't loaded.\n";
		}
		#ifdef NS_OUTPUT_ALGINMENT_DEBUG
		if (i == 34)
			std::cerr << "WHA";
		#endif
		elements[i].registration_offset = align(state,elements[i].path_aligned_images->image,elements[i].saturated_offset);
		#ifdef NS_OUTPUT_ALGINMENT_DEBUG
		cerr << "Alignment: " << elements[i].registration_offset << "\n";
		#endif
		state.registration_offset_sum += (elements[i].registration_offset - elements[i-step*time_kernal].registration_offset);

		for (long y = d.y; y < h_sub.y; y++){
			for (long x = d.x; x < h_sub.x; x++){
				state.consensus		 [y][x]+= elements[i            ].path_aligned_images->image.sample_f(y-elements[i].registration_offset.y,x-elements[i].registration_offset.x);
				//state.consensus_count[y][x]++;

				state.consensus		 [y][x]-= elements[i-step*time_kernal].path_aligned_images->image.sample_f(y-elements[i-step*time_kernal].registration_offset.y,x-elements[i-step*time_kernal].registration_offset.x);
				//state.consensus_count[y][x]--;
			
			}
		}
		#ifdef NS_OUTPUT_ALGINMENT_DEBUG
		ns_debug_image_out out;
		out("c:\\movement\\",debug_path_name + ns_to_string(i) + ".tif",alignment_time_kernel_width,state.consensus,state.consensus_count,elements[i].path_aligned_images->image);
		#endif
	}
}

template<class T> 
struct ns_index_orderer{
	ns_index_orderer<T>(const unsigned long i,const T * d):index(i),data(d){}
	bool operator<(const ns_index_orderer<T> & r) const {return *data < *r.data;}
	unsigned long index;
	const T * data;
};

ns_analyzed_image_time_path_group::ns_analyzed_image_time_path_group(unsigned long group_id_, const unsigned long region_info_id,const ns_time_path_solution & solution_, const ns_death_time_annotation_time_interval & observation_time_interval,ns_death_time_annotation_set & rejected_annotations,ns_time_path_image_movement_analysis_memory_pool & memory_pool){
	
	paths.reserve(solution_.path_groups[group_id_].path_ids.size());
	unsigned long current_path_id(0);

	//we find when the first and last observations of the plate were made
	//these are needed for generating proper event times when those occur at the edges of observation
	ns_time_path_limits limits;
	limits.first_obsevation_of_plate.period_start = ULONG_MAX;
	limits.first_obsevation_of_plate.period_end = ULONG_MAX;
	limits.last_obsevation_of_plate.period_start = 0;
	limits.last_obsevation_of_plate.period_end = 0;
	for (unsigned int i = 0; i < solution_.timepoints.size(); i++){
		if (solution_.timepoints[i].time < observation_time_interval.period_start || 
			solution_.timepoints[i].time > observation_time_interval.period_end)
			continue;
		if (limits.first_obsevation_of_plate.period_start == ULONG_MAX)
			limits.first_obsevation_of_plate.period_start = solution_.timepoints[i].time;
		else if (limits.first_obsevation_of_plate.period_end == ULONG_MAX){
			limits.first_obsevation_of_plate.period_end = solution_.timepoints[i].time;
		}
		if (limits.last_obsevation_of_plate.period_end < solution_.timepoints[i].time){
			limits.last_obsevation_of_plate.period_start = limits.last_obsevation_of_plate.period_end;
			limits.last_obsevation_of_plate.period_end = solution_.timepoints[i].time;
		}
	}

	//if (group_id_ == 28)
	//			cerr << "MA";
	//if (limits.last_obsevation_of_plate.periode

	for (unsigned int i = 0; i < solution_.path_groups[group_id_].path_ids.size(); i++){
		const unsigned long & path_id(solution_.path_groups[group_id_].path_ids[i]);
		const ns_time_path & source_path(solution_.paths[path_id]);
		paths.resize(current_path_id+1,ns_analyzed_image_time_path(memory_pool));
		ns_analyzed_image_time_path &path(paths[current_path_id]);

		path.path = &source_path;
		path.path_id = path_id;
		path.group_id = group_id_;
		path.solution = &solution_;
		path.region_info_id = region_info_id;
		
		if (source_path.stationary_elements.size() == 0)
			throw ns_ex("ns_analyzed_image_time_path_group::ns_analyzed_image_time_path_group()::Empty timepath found!");
		
		vector<ns_index_orderer<unsigned long> > ordered_time;
		ordered_time.reserve(source_path.stationary_elements.size());
		for (unsigned int j = 0; j < source_path.stationary_elements.size(); j++)
			ordered_time.push_back(ns_index_orderer<unsigned long>(j,&solution_.time(source_path.stationary_elements[j])));
		std::sort(ordered_time.begin(),ordered_time.end());
		
	//	ns_time_path_limits limits;
		//set the time of the first measurement after the path ends.
		//this is used to annotate the time at which the path disappears
		const unsigned long last_time_index(source_path.stationary_elements[ordered_time.rbegin()->index].t_id);
		const unsigned long first_time_index(source_path.stationary_elements[ordered_time.begin()->index].t_id);

		if (last_time_index+1 >= solution_.timepoints.size() ||
			solution_.timepoints[last_time_index].time > limits.last_obsevation_of_plate.period_start
			){
			limits.interval_after_last_observation.period_start = solution_.timepoints.rbegin()->time;
			limits.interval_after_last_observation.period_end_was_not_observed = true;
		}
		else limits.interval_after_last_observation = ns_death_time_annotation_time_interval(solution_.timepoints[last_time_index].time,
																							solution_.timepoints[last_time_index+1].time);
		if (first_time_index < 1 ||
			solution_.timepoints[first_time_index].time < limits.first_obsevation_of_plate.period_start){
			limits.interval_before_first_observation.period_start_was_not_observed = true;
			limits.interval_before_first_observation.period_end = solution_.timepoints[0].time;
		}
		else limits.interval_before_first_observation = ns_death_time_annotation_time_interval(solution_.timepoints[first_time_index-1].time,
																							solution_.timepoints[first_time_index].time);
		
		path.set_time_path_limits(limits);			
			
		path.elements.reserve(source_path.stationary_elements.size());
		for (unsigned int j = 0; j < source_path.stationary_elements.size(); j++){
			
			const unsigned long s = path.elements.size();
			const unsigned long absolute_time(solution_.time(source_path.stationary_elements[ordered_time[j].index]));
			if (s > 0 && path.elements[s-1].absolute_time == absolute_time){
				if (path.elements[s-1].excluded) continue;
				//add an annotation that there is an extra worm at the specified position
				/*std::string expl("PD");
				rejected_annotations.events.push_back(
					 ns_death_time_annotation(ns_no_movement_event,
					 0,region_info_id,
					 ns_death_time_annotation_time(solution_.time(source_path.stationary_elements[ordered_time[j].index]),solution_.time(source_path.stationary_elements[ordered_time[j].index])),
					 solution_.element(source_path.stationary_elements[ordered_time[j].index]).region_position,
					 solution_.element(source_path.stationary_elements[ordered_time[j].index]).region_size,
					 ns_death_time_annotation::ns_machine_excluded,
					 ns_death_time_annotation_event_count(2,0),ns_current_time(),
					 ns_death_time_annotation::ns_lifespan_machine,
					 (solution_.element(source_path.stationary_elements[ordered_time[j].index]).part_of_a_multiple_worm_disambiguation_cluster)?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
					 ns_stationary_path_id(),false,
					 expl)
				);*/
				path.elements[s-1].number_of_extra_worms_observed_at_position++;
				continue; //skip frames taken at duplicate times
			}

			path.elements.resize(s+1);
			path.elements[s].absolute_time = absolute_time;
			path.elements[s].relative_time = path.elements[s].absolute_time - path.elements[0].absolute_time;
			path.elements[s].context_position_in_region_vis_image = solution_.element(source_path.stationary_elements[ordered_time[j].index]).context_image_position_in_region_vis_image;
			path.elements[s].inferred_animal_location = solution_.element(source_path.stationary_elements[ordered_time[j].index]).inferred_animal_location;
			path.elements[s].element_before_fast_movement_cessation = solution_.element(source_path.stationary_elements[ordered_time[j].index]).element_before_fast_movement_cessation;
	//		if (path.elements[s].element_before_fast_movement_cessation)
	//			cerr << "WHA";
			path.elements[s].worm_region_size_ = solution_.element(source_path.stationary_elements[ordered_time[j].index]).region_size;
			path.elements[s].worm_context_size_ = solution_.element(source_path.stationary_elements[ordered_time[j].index]).context_image_size;

			path.elements[s].part_of_a_multiple_worm_disambiguation_group = solution_.element(source_path.stationary_elements[ordered_time[j].index]).part_of_a_multiple_worm_disambiguation_cluster;
			//we inherited a path_position_offset offset due to the extra margin
			//of context pixels included in the region_visualization image.
			//thus, the absolute position of all our pixels are actually offset
			path.elements[s].region_position_in_source_image = 
				solution_.element(source_path.stationary_elements[ordered_time[j].index]).region_position;
			path.elements[s].context_position_in_source_image = 
				solution_.element(source_path.stationary_elements[ordered_time[j].index]).context_image_position;
		  
			//we'll subtract out the path location later 
		//	path.elements[s].offset_from_path =  path.elements[s].context_position_in_source_image;
		}
		path.find_first_labeled_stationary_timepoint();
		//if this path was shrunk down below the minimum size for analysis, remove it.
		const unsigned long current_time(ns_current_time());
		if (path.elements.size() < ns_analyzed_image_time_path::alignment_time_kernel_width){
		//	cerr << "Path size : " << path.elements.size() << "\n";
			for (unsigned int j = 0; j < path.elements.size(); j++){
				
				if (path.elements[j].excluded || path.elements[j].inferred_animal_location) continue;
				 rejected_annotations.events.push_back(
					 ns_death_time_annotation(ns_fast_moving_worm_observed,
					 0,region_info_id,
					 ns_death_time_annotation_time_interval(path.elements[j].absolute_time,path.elements[j].absolute_time),
					 path.elements[j].region_offset_in_source_image(),
					 path.elements[j].worm_region_size(),
					 ns_death_time_annotation::ns_not_excluded,
					 ns_death_time_annotation_event_count(1+path.elements[j].number_of_extra_worms_observed_at_position,0),current_time,ns_death_time_annotation::ns_lifespan_machine,
					 path.elements[j].part_of_a_multiple_worm_disambiguation_group?ns_death_time_annotation::ns_part_of_a_mutliple_worm_disambiguation_cluster:ns_death_time_annotation::ns_single_worm,
					 ns_stationary_path_id(),false,
					 "PTS")
					 );
			}
			path.elements.resize(0);
			paths.pop_back();
			continue;
		}
		
		//first we calculate the path bounding box in absolute coordinates
		ns_vector_2i tl(path.elements[0].context_position_in_source_image),
					 br(path.elements[0].context_position_in_source_image+path.elements[0].worm_context_size_);
		
		ofstream * debug(0);
/*		if (0 && group_id_==42){
			string fname("c:\\path_foo_");
			srand(ns_current_time());
			fname += ns_to_string(rand());
			fname += ".tif";
			debug = new ofstream(fname.c_str());
			*debug << "TLx,TLy,BRx,BRy,|,POSx,POSY,POS+SIZEX,POS+SIZEY\n";
		}*/
		for (unsigned int j = 1; j < path.elements.size(); j++){
			const ns_analyzed_image_time_path_element & e(path.elements[j]);
			const ns_vector_2i & pos(e.context_position_in_source_image);
			if (pos.x == -1 || pos.y == -1)
				throw ns_ex("An unspecified context position was found in the time path solution");
			const ns_vector_2i & size(e.worm_context_size_);
			if (debug != 0){
				*debug << tl << "," << br << ",|," << pos << "," << pos+size << "\n";
			}
			if (pos.x < tl.x)
				tl.x = pos.x;
			if (pos.y < tl.y)
				tl.y = pos.y;
			if (pos.x + size.x > br.x)
				br.x = pos.x + size.x;
			if (pos.y + size.y > br.y)
				br.y = pos.y + size.y; 
		}
		if (debug != 0)
			debug->close();
		//the absolute coordinates of the path
		path.path_context_position = tl;
		path.path_context_size = br - tl;

		//we want to map from region positions to context positions
		path.path_region_position = tl + solution_.element(source_path.stationary_elements[ordered_time[0].index]).region_position  
													 - solution_.element(source_path.stationary_elements[ordered_time[0].index]).context_image_position;
		path.path_region_size = path.path_context_size + solution_.element(source_path.stationary_elements[ordered_time[0].index]).region_size
												 - solution_.element(source_path.stationary_elements[ordered_time[0].index]).context_image_size;
		if (path.path_region_position==ns_vector_2i(2579,300))
				cerr << "WHA";
	//	path.worm_size = path.region_size;

		for(unsigned int j = 0; j < path.elements.size(); j++){
			path.elements[j].offset_from_path = path.elements[j].context_position_in_source_image - path.path_context_position;
				if (path.elements[j].region_offset_in_source_image()==ns_vector_2i(2579,300))
				cerr << "WHA";
				if (path.elements[j].region_offset_in_source_image()==ns_vector_2i(2579,301))
				cerr << "WHA";
				if (path.elements[j].region_offset_in_source_image()==ns_vector_2i(2579,301) && path.elements[j].absolute_time == 1408897604)
				cerr << "WHA";
		}
		current_path_id++;
	}
}

void ns_movement_image_collage_info::from_path(const ns_analyzed_image_time_path * p, const unsigned long only_output_first_n_lines){
	const unsigned long max_width(3000);

	if (p->element_count() == 0){
		frame_dimensions = ns_vector_2i(0,0);
		grid_dimensions = ns_vector_2i(0,0);
		prop.width = 0;
		prop.height = 0;
		prop.components = 0;
		prop.resolution = 0;
		return;
	}
	ns_image_properties pp;
	p->set_path_alignment_image_dimensions(pp);
	frame_dimensions.x = pp.width;
	frame_dimensions.y = pp.height;
	//frame_dimensions = p->path_context_size;
	if (frame_dimensions.x > max_width)
		throw ns_ex("ns_movement_image_collage_info()::Frame too large!");

	//int frames_per_row(max_width/frame_dimensions.x);
	int frames_per_row = 1;
	//we could get a lot nicer images by having multiple frames per row.
	//however, this would mean we'd not be able to output frames one at a time, which would complicate things.

	//if (frame_dimensions.x*p.element_count() < max_width)
	//	frames_per_row = p.element_count();
	unsigned long frame_count(only_output_first_n_lines);
	if (frame_count == 0)
		frame_count= p->element_count();
	grid_dimensions = ns_vector_2i(frames_per_row,frame_count/frames_per_row  + (unsigned long)((frame_count % frames_per_row)>0));

	prop = p->registered_image_properties;

	prop.width = grid_dimensions.x*frame_dimensions.x;
	prop.height = grid_dimensions.y*frame_dimensions.y;
	//if (prop.height > 1500000)
	//	throw ns_ex("ns_movement_image_collage_info::Movement collage would require an enormous image:") << prop.width << "x" << prop.height;
	prop.components = 3;
//	prop.description = xml.result();
//		cerr << p.element_count() << ": " << frames_per_row << " " << grid_dimensions << " " << frame_dimensions << " " <<
//			prop.width << "x" << prop.height << "\n";
}


void ns_analyzed_image_time_path::save_movement_images(const ns_analyzed_time_image_chunk & chunk,ns_sql & sql){
	//handle small or non-existant images
	ns_movement_image_collage_info d(this
		/*,chunk.stop_i-chunk.start_i*/);
	if(d.prop.width == 0 || d.prop.height == 0){
		if (chunk.start_i > 0)
			return;
		ns_image_properties p(d.prop);
		p.height = 1;
		p.width = 1;
		if (p.components == 0)
			p.components = 3;
		ns_image_standard im;
		im.init(p);
		for (unsigned int c = 0; c < p.components; c++)
			im[0][c] = 0;	
		bool had_to_use_volatile_storage;
		output_reciever = new ns_image_storage_reciever_handle<ns_8_bit>(image_server.image_storage.request_storage(output_image,ns_tiff,1024,&sql,had_to_use_volatile_storage,false,false));	
		im.pump(output_reciever->output_stream(),1024);
		return;
	}

	if (chunk.start_i == 0){
		bool had_to_use_volatile_storage;
		output_reciever = new ns_image_storage_reciever_handle<ns_8_bit>(image_server.image_storage.request_storage(output_image,ns_tiff,1024,&sql,had_to_use_volatile_storage,false,false));
				
		output_reciever->output_stream().init(d.prop);
	}
	save_movement_images(chunk,*output_reciever);
	/*for (unsigned int i = chunk.start_i; i < chunk.stop_i; i++){
		elements[i].registered_images->image.clear();
		elements[i].registered_images->movement_image_.clear();
		elements[i].registered_images->region_threshold.clear();
		elements[i].registered_images->worm_threshold_.clear();
		elements[i].registered_images->worm_neighborhood_threshold.clear();
	}*/

	if (
	///**/	true || 
		chunk.stop_i == elements.size()){
		output_reciever->output_stream().finish_recieving_image();
		delete output_reciever;
		output_reciever = 0;
	//**/	exit(0);
	}
}
void ns_analyzed_image_time_path::save_movement_images(const ns_analyzed_time_image_chunk & chunk,ns_image_storage_reciever_handle<ns_8_bit> & out){
	
	if (elements.size() == 0) return;

	ns_movement_image_collage_info d(this);
	//this is the range within the entire path image that we are currently writing out
	unsigned long start_y = d.frame_dimensions.y*(chunk.start_i/d.grid_dimensions.x);
	unsigned long stop_y = d.frame_dimensions.y*((chunk.stop_i-1)/d.grid_dimensions.x + 1);

	//this is the size of the buffer we need to allocate to store the current chunk of the entire path image
	ns_image_stream_buffer_properties p;
	const unsigned long chunk_height = stop_y - start_y;
	p.height = chunk_height;
	//don't allocate too much otherwise we might hit the memmory ceiling and/or stress out libtiff
	if (p.height > 512)
		p.height = 512;

	const unsigned long h(p.height);
	p.width = d.prop.width*d.prop.components;
	save_image_buffer.resize(p);

	//cerr << start_y << "-" << stop_y << " by " << h << "\n";
	//we output the range of the entire path image in h sized chunks
	for (unsigned long y = start_y; y < stop_y; y+=h){
	//	cerr <<y << "-> " << y + h << " modified to ";
		
		unsigned long dh(h);	
		if (dh + y > d.prop.height) dh = d.prop.height-y;
		if (dh + y > stop_y) dh = stop_y - y;
	//	cerr << y << "-> " << y + dh << "\n ";

		for (unsigned long dy = 0; dy < dh; dy++){
			//cy is the current line of the output buffer we are writing
			const unsigned long cy(y+dy);
			//i_offset is the index of the first path element we are going to write on this line of the output_buffer
			const unsigned long i_offset = d.grid_dimensions.x*(cy/d.frame_dimensions.y);
			//y_im_offset is the y within the current path element that we want to write.
			const unsigned long y_im_offset = cy%d.frame_dimensions.y;

			for (unsigned long i = 0; i < d.grid_dimensions.x; i++){
				//l_x is the position on the current line that we want to write the path element i+i_offset
				const unsigned long l_x(i*d.frame_dimensions.x);
				//r_x is one beyond the position on the current line that we want to write the path element i+i_offset
				const unsigned long r_x((i+1)*d.frame_dimensions.x);
				//if we've outputted all the elements; fill in the rest black.
				if (i_offset+i >= elements.size()){
					for (unsigned int x = 3*l_x; x < 3*r_x; x++)	
						save_image_buffer[dy][x] =  0;
	
				}
				else{

					const ns_analyzed_image_time_path_element &e(elements[i+i_offset]);	
					if (e.excluded)
						for (unsigned int x = l_x; x < r_x; x++){
							save_image_buffer[dy][3*x] = 0;
							save_image_buffer[dy][3*x+1] = 0;
							save_image_buffer[dy][3*x+2] = 0;
						}
					else{
						for (unsigned int x = l_x; x < r_x; x++){
							save_image_buffer[dy][3*x] =  abs(e.registered_images->movement_image_[y_im_offset][x-l_x]);  //unsigned value of movement difference.
							save_image_buffer[dy][3*x+1] =  e.registered_images->image[y_im_offset][x-l_x];
							save_image_buffer[dy][3*x+2] =   (((ns_8_bit)((e.registered_images->movement_image_[y_im_offset][x-l_x]<0)?1:0))) | //sign of movement difference
															 (((ns_8_bit)(e.registered_images->get_worm_neighborhood_threshold(y_im_offset,x-l_x)?1:0))<<3) |
															 (((ns_8_bit)(e.registered_images->get_worm_threshold(y_im_offset,x-l_x)?1:0))<<4) | 
															 (((ns_8_bit)(e.registered_images->get_region_threshold(y_im_offset,x-l_x)?1:0))<<5);
												;		
						}
					}
				}
			}
		}
	//	cerr << y << "->" << dh << "\n";
		out.output_stream().recieve_lines(save_image_buffer,dh);
	}
		
}


void ns_analyzed_image_time_path::initialize_movement_image_loading(ns_image_storage_source_handle<ns_8_bit> & in){
	if (elements.size() == 0) return;
	movement_loading_collage_info.from_path(this);


	if (in.input_stream().properties().width != movement_loading_collage_info.prop.width ||
		in.input_stream().properties().height != movement_loading_collage_info.prop.height ||
		in.input_stream().properties().components != movement_loading_collage_info.prop.components){
			throw ns_ex("ns_analyzed_image_time_path::load_movement_images_from_file()::Unexpected collage size: ")
				<< in.input_stream().properties().width << "x" << in.input_stream().properties().height << ":" << (long)in.input_stream().properties().components << "," << (long)in.input_stream().properties().resolution << " vs "
				<< movement_loading_collage_info.prop.width << "x" << movement_loading_collage_info.prop.height << ":" << movement_loading_collage_info.prop.components << "," << movement_loading_collage_info.prop.resolution << "\n";
	}
	
	ns_image_stream_buffer_properties p;
	p.height = 2048;
	if (p.height > movement_loading_collage_info.prop.height)
		p.height = movement_loading_collage_info.prop.height;
	p.width = movement_loading_collage_info.prop.width*movement_loading_collage_info.prop.components;
	movement_loading_buffer.resize(p);

}
void ns_analyzed_image_time_path::end_movement_image_loading(ns_image_storage_source_handle<ns_8_bit> & in){
		movement_loading_buffer.resize(ns_image_stream_buffer_properties(0,0));
}

void ns_analyzed_image_time_path::load_movement_images(const ns_analyzed_time_image_chunk & chunk,ns_image_storage_source_handle<ns_8_bit> & in){
	
	ns_image_stream_buffer_properties p(movement_loading_buffer.properties());
	const unsigned long h(p.height);

	for (unsigned int i = chunk.start_i; i < chunk.stop_i; i++){
		ns_image_properties pr(in.input_stream().properties());
		set_path_alignment_image_dimensions(pr);
		elements[i].initialize_registered_images(pr,memory_pool->registered_image_pool);
		/*elements[i].registered_images->movement_image_.init(pr);
		elements[i].registered_images->image.init(pr);
		elements[i].registered_images->worm_threshold_.init(pr);
		elements[i].registered_images->worm_neighborhood_threshold.init(pr);
		elements[i].registered_images->region_threshold.init(pr);*/
	}

	unsigned long start_y(movement_loading_collage_info.frame_dimensions.y*(chunk.start_i/movement_loading_collage_info.grid_dimensions.x));
	
	unsigned long stop_y(movement_loading_collage_info.frame_dimensions.y*(chunk.stop_i/movement_loading_collage_info.grid_dimensions.x));

	for (unsigned long y = start_y; y < stop_y; y+=h){
		
		unsigned long dh(h);	
		if(h + y > stop_y) dh = stop_y-y;
		in.input_stream().send_lines(movement_loading_buffer,dh);

		for (unsigned long dy = 0; dy < dh; dy++){
			const unsigned long cy(y+dy);
			const unsigned long i_offset = movement_loading_collage_info.grid_dimensions.x*(cy/movement_loading_collage_info.frame_dimensions.y);
			const unsigned long y_im_offset = cy%movement_loading_collage_info.frame_dimensions.y;
			for (unsigned long i = 0; i < movement_loading_collage_info.grid_dimensions.x; i++){
				const unsigned long l_x(i*movement_loading_collage_info.frame_dimensions.x);
				const unsigned long r_x((i+1)*movement_loading_collage_info.frame_dimensions.x);
				//if we've outputted all the elements; fill in the rest black.
				if (i_offset+i >= chunk.stop_i)
					throw ns_ex("YIKES!");

				ns_analyzed_image_time_path_element &e(elements[i+i_offset]);	
	//			cerr << "" << i + i_offset << ",";
			
				for (unsigned int x = l_x; x < r_x; x++){
					//unsigned value is stored in channel 0.  the sign of the value is stored in the first bit of channel 3
					 e.registered_images->movement_image_[y_im_offset][x-l_x] = movement_loading_buffer[dy][3*x] * (((movement_loading_buffer[dy][3*x+2])&1)?-1:1);
					 e.registered_images->image[y_im_offset][x-l_x] = movement_loading_buffer[dy][3*x+1];
					 e.registered_images->set_thresholds(y_im_offset,x-l_x,
						 /* e.registered_images->region_threshold[y_im_offset][x-l_x]*/ ((movement_loading_buffer[dy][3*x+2])&(((ns_8_bit)1)<<5))>0,
				
					 /*e.registered_images->worm_threshold_[y_im_offset][x-l_x] =  */ ((movement_loading_buffer[dy][3*x+2])&(((ns_8_bit)1)<<4))>0,
						
					/* e.registered_images->worm_neighborhood_threshold[][] =*/   ((movement_loading_buffer[dy][3*x+2])&(((ns_8_bit)1)<<3))>0);
					}
			}
		}
	//	cerr << "(" << y << "->" << dh << ")\n";
	}
}

bool ns_time_path_image_movement_analyzer::load_movement_image_db_info(const unsigned long region_id,ns_sql & sql){
	if (image_db_info_loaded)
		return true;
	sql << "SELECT group_id,path_id,image_id, id FROM path_data WHERE region_id = " << region_id;
	ns_sql_result res;
	sql.get_rows(res);
	if (res.size() == 0){
			image_db_info_loaded = false;
			return false;
	}
	for (unsigned int i = 0; i < res.size(); i++){
		unsigned long group_id(atol(res[i][0].c_str())),
					  path_id(atol(res[i][1].c_str())),
					  image_id(atol(res[i][2].c_str())),
					  db_id(atol(res[i][3].c_str()));

		if (group_id >= groups.size() || path_id >= groups[group_id].paths.size()){
			sql << "DELETE from path_data WHERE id = " << db_id;
			sql.send_query();
		}
		else{
			groups[group_id].paths[path_id].output_image.id = image_id;
			if (image_id == 0)
				cerr << "Zero Image Id";
			groups[group_id].paths[path_id].path_db_id = db_id;
		}
		
	}
	image_db_info_loaded = true;
	return true;
}

void ns_time_path_image_movement_analyzer::load_images_for_group(const unsigned long group_id, unsigned long number_of_images_to_load,ns_sql & sql, const bool load_images_after_last_valid_sample){
	if (groups[group_id].paths.size() == 0)
		return;
	const unsigned long number_of_images_loaded(groups[group_id].paths[0].number_of_images_loaded);
	if (number_of_images_loaded == 0){
		(region_info_id,sql); // TODO: LOLWHUT? Is this supposed to do something?
		if (group_id >= size())
			throw ns_ex("ns_time_path_image_movement_analyzer::load_images_for_group()::Invalid group: ") << group_id;
	}
	if (number_of_images_loaded >= number_of_images_to_load)
		return;

	const unsigned long chunk_size(10);

	for (unsigned int j = 0; j < groups[group_id].paths.size(); j++){
		unsigned long number_of_valid_elements(0);
		if (load_images_after_last_valid_sample)
			number_of_valid_elements = groups[group_id].paths[j].elements.size();
		else{
			for (int k = (int)groups[group_id].paths[j].elements.size()-1; k >= number_of_images_loaded; k--){
				if (!groups[group_id].paths[j].elements[k].excluded){
					number_of_valid_elements = k+1;
					break;
				}
			}
		}
		
		if (number_of_images_loaded == 0){
			if (groups[group_id].paths[j].output_image.id==0){
				throw ns_ex("ns_time_path_image_movement_analyzer::load_images_for_group()::Group has no stored image id specified");
			}
			groups[group_id].paths[j].output_image.load_from_db(groups[group_id].paths[j].output_image.id,&sql);
			groups[group_id].paths[j].movement_image_storage = image_server.image_storage.request_from_storage(groups[group_id].paths[j].output_image,&sql);
			groups[group_id].paths[j].initialize_movement_image_loading(groups[group_id].paths[j].movement_image_storage);
		}

		
		if (number_of_images_to_load > number_of_valid_elements)
			number_of_images_to_load = number_of_valid_elements;
			//throw ns_ex("ns_time_path_image_movement_analyzer::load_images_for_group()::Requesting to many images!");
		if (number_of_images_to_load == 0)
			return;

		unsigned long number_of_new_images_to_load(number_of_images_to_load-number_of_images_loaded);
			
		//load in chunk by chunk
		for (unsigned int k = 0; k < number_of_new_images_to_load; k+=chunk_size){
			ns_analyzed_time_image_chunk chunk(k,k+chunk_size);
			if (chunk.stop_i >= number_of_new_images_to_load)
				chunk.stop_i = number_of_new_images_to_load;
			chunk.start_i+=number_of_images_loaded;
			chunk.stop_i+=number_of_images_loaded;

					
			groups[group_id].paths[j].load_movement_images(chunk,groups[group_id].paths[j].movement_image_storage);
//				groups[i].paths[j].save_movement_images(chunk,dbg_im);
		//	groups[i].paths[j].quantify_movement(chunk);
		//	for (long l = chunk.start_i; l < (long)chunk.stop_i; l++)
		//		groups[i].paths[j].elements[l].clear_movement_images();
		}
		if (number_of_images_to_load == number_of_valid_elements){
			groups[group_id].paths[j].end_movement_image_loading(groups[group_id].paths[j].movement_image_storage);
			groups[group_id].paths[j].movement_image_storage.clear();
		}
		groups[group_id].paths[j].number_of_images_loaded = number_of_images_to_load;
		//groups[i].paths[j].analyze_movement(persistance_time,ns_stationary_path_id(i,j,analysis_id));
			
	//	debug_name += ".csv";
	//	ofstream tmp(debug_name.c_str());
	//	groups[i].paths[j].output_image_movement_summary(tmp);
	}
}

void ns_time_path_image_movement_analyzer::clear_images_for_group(const unsigned long group_id){
	if (groups.size() <= group_id)
		return;
	for (unsigned int j = 0; j < groups[group_id].paths.size(); j++){
		for (unsigned int k = 0; k < groups[group_id].paths[j].elements.size(); k++){
			groups[group_id].paths[j].elements[k].clear_movement_images();
		}
		groups[group_id].paths[j].number_of_images_loaded = 0;
	}
}


void ns_time_path_image_movement_analyzer::reanalyze_stored_aligned_images(const ns_64_bit region_id,const ns_time_path_solution & solution_,const ns_time_series_denoising_parameters & times_series_denoising_parameters,const ns_analyzed_image_time_path_death_time_estimator * e,ns_sql & sql,const bool load_images_after_last_valid_sample){
	const unsigned long chunk_size(10);
	analysis_id = ns_current_time();
	try{
		region_info_id = region_id;
		
		externally_specified_plate_observation_interval = get_externally_specified_last_measurement(region_id,sql);
		load_from_solution(solution_);
		crop_path_observation_times(externally_specified_plate_observation_interval);

		load_movement_image_db_info(region_id,sql);
		
		for (unsigned int i = 0; i < groups.size(); i++)
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
					continue;
				if (groups[i].paths[j].output_image.id == 0)
					throw ns_ex("Could not find movement image in db for region ") << region_id << " group " << i << " path " << j;
			}

		calculate_memory_pool_maximum_image_size(0,groups.size());

		//we need to load in the movement quantification file because it holds the registration offsets used to generate
		//the registered movement images, and these values need to be written to the new movement quantification file.
		populate_movement_quantification_from_file(sql);

		for (unsigned int i = 0; i < groups.size(); i++){
			cout << (100*i)/groups.size() << "%...";
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
					continue;
				groups[i].paths[j].output_image.load_from_db(groups[i].paths[j].output_image.id,&sql);
				ns_image_storage_source_handle<ns_8_bit> storage(image_server.image_storage.request_from_storage(groups[i].paths[j].output_image,&sql));
				groups[i].paths[j].initialize_movement_image_loading(storage);


				unsigned long number_of_valid_elements(0);
				if (load_images_after_last_valid_sample)
					number_of_valid_elements = groups[i].paths[j].elements.size();
				else{
					for (int k = (int)groups[i].paths[j].elements.size()-1; k >= 0; k--){
						if (!groups[i].paths[j].elements[k].excluded){
							number_of_valid_elements = k+1;
							break;
						}
					}
				}


				//load in chunk by chunk
				for (unsigned int k = 0; k < number_of_valid_elements; k+=chunk_size){
					ns_analyzed_time_image_chunk chunk(k,k+chunk_size);
					if (chunk.stop_i >= number_of_valid_elements)
						chunk.stop_i = number_of_valid_elements;
					
					groups[i].paths[j].load_movement_images(chunk,storage);
	//				groups[i].paths[j].save_movement_images(chunk,dbg_im);
					groups[i].paths[j].quantify_movement(chunk);
					for (long l = chunk.start_i; l < (long)chunk.stop_i; l++)
						groups[i].paths[j].elements[l].clear_movement_images();
				}
				groups[i].paths[j].end_movement_image_loading(storage);
				groups[i].paths[j].denoise_movement_series(times_series_denoising_parameters);
			
				//groups[i].paths[j].analyze_movement(e,generate_stationary_path_id(i,j));
				
				//groups[i].paths[j].calculate_movement_quantification_summary();
			//	debug_name += ".csv";
			//	ofstream tmp(debug_name.c_str());
			//	groups[i].paths[j].output_image_movement_summary(tmp);
			}
		}
		normalize_movement_scores_over_all_paths(times_series_denoising_parameters);

		for (unsigned int i = 0; i < groups.size(); i++){
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
					continue;
				groups[i].paths[j].analyze_movement(e,generate_stationary_path_id(i,j),last_timepoint_in_analysis_);
				groups[i].paths[j].calculate_movement_quantification_summary();
			}
		}
		memory_pool.clear();
		cout << "\n";
		generate_movement_description_series();
		movement_analyzed = true;
	}
	catch(...){
		delete_from_db(region_id,sql);
		throw;
	}
}
void ns_time_path_image_movement_analyzer::mark_path_images_as_cached_in_db(const unsigned long region_id, ns_sql & sql){
	sql << "UPDATE sample_region_image_info SET path_movement_images_are_cached = 1 WHERE id = " << region_id;
	sql.send_query();
}
void ns_time_path_image_movement_analyzer::acquire_region_image_specifications(const unsigned long region_id,ns_sql & sql){
	const string col(ns_processing_step_db_column_name(ns_process_region_vis));
	const string col2(ns_processing_step_db_column_name(ns_process_region_interpolation_vis));
	sql << "SELECT " << col << "," << col2 << ",capture_time, worm_detection_results_id, worm_interpolation_results_id, id "
		   "FROM sample_region_images WHERE region_info_id = " << region_id << " AND (" << col << "!= 0 || " << col2 << "!=0) ORDER BY capture_time ASC";
	ns_sql_result res;
	sql.get_rows(res);
	region_image_specifications.resize(res.size());
	for (unsigned long i = 0; i < res.size(); i++){
		//load image
		region_image_specifications[i].time = ns_atoi64(res[i][2].c_str());
		region_image_specifications[i].sample_region_image_id = ns_atoi64(res[i][5].c_str());
		region_image_specifications[i].region_vis_image.load_from_db(ns_atoi64(res[i][0].c_str()),&sql);
		ns_64_bit id(ns_atoi64(res[i][1].c_str()));
		if (id != 0)
			region_image_specifications[i].interpolated_region_vis_image.load_from_db(id,&sql);
		else region_image_specifications[i].interpolated_region_vis_image.id = 0;
	}
	//make sure we only use time point data specified in the solution
	std::vector<ns_analyzed_image_specification>::iterator p = region_image_specifications.begin();
	for (unsigned int i = 0; i < solution->timepoints.size();i++){
		if (p == region_image_specifications.end() || solution->timepoints[i].time < p->time)
			throw ns_ex("ns_time_path_image_movement_analyzer::acquire_region_image_specifications()::Could not identify all region images specified in the time path solution");
		while(p->time < solution->timepoints[i].time)
			p = region_image_specifications.erase(p);
		if (p->time == solution->timepoints[i].time)
			p++;
	}
}

void ns_time_path_image_movement_analyzer::normalize_movement_scores_over_all_paths(const ns_time_series_denoising_parameters & param){
	if (param.movement_score_normalization == ns_time_series_denoising_parameters::ns_none ||
		param.movement_score_normalization == ns_time_series_denoising_parameters::ns_subtract_out_device_median)
		return;

	denoising_parameters_used = param;

	std::vector<double> medians;
	std::vector<char> excluded_from_plate_measurements;
	unsigned long total_measurements(0);
	for (unsigned int i = 0; i < groups.size(); i++){
		for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
						continue;

			ns_analyzed_image_time_path::ns_element_list & elements(groups[i].paths[j].elements);
			
						
			//find median of last 15% of curve
			unsigned long start;
			if (param.movement_score_normalization == ns_time_series_denoising_parameters::ns_subtract_out_median_of_end ||
				param.movement_score_normalization == ns_time_series_denoising_parameters::ns_subtract_out_plate_median)
				start = (17*elements.size())/20;
			else if (param.movement_score_normalization == ns_time_series_denoising_parameters::ns_subtract_out_median)
				start = 0;
			else throw ns_ex("Unknown movement score normalization scheme:") << (long)(param.movement_score_normalization);
			if (elements.size()-start < 2)
				start = 0;

			std::vector<double> values(elements.size()-start);
			for (unsigned int k = start; k < elements.size(); k++){
				values[k-start] = elements[k].measurements.death_time_posture_analysis_measure();
			}

			if (elements.size() < 10 || groups[i].paths[j].excluded())
				excluded_from_plate_measurements.push_back(1);
			else excluded_from_plate_measurements.push_back(0);

			std::sort(values.begin(),values.end());
			if (values.size()%2 == 1)
				medians.push_back(values[values.size()/2]);
			else
				medians.push_back((values[values.size()/2]+values[values.size()/2-1])/2.0);
		}
	}
	if (param.movement_score_normalization == ns_time_series_denoising_parameters::ns_subtract_out_plate_median){
		//weighted median of all measurements made of all animals detected on the plate.
		std::vector<double> medians_to_use;
		for (unsigned int i = 0; i < medians.size(); i++){
			if (!excluded_from_plate_measurements[i])
				medians_to_use.push_back(medians[i]);
		}
		std::sort(medians_to_use.begin(),medians_to_use.end());
		double median;
		if (medians_to_use.empty())
			median = 0;
		else{
			if (medians_to_use.size()%2 == 1)
				median = medians_to_use[medians_to_use.size()/2];
			else
				median = (medians_to_use[medians_to_use.size()/2]+medians_to_use[medians_to_use.size()/2-1])/2.0;
		}
		for (unsigned int i = 0; i < groups.size(); i++){
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
						continue; 	

				for (unsigned int k = 0; k < groups[i].paths[j].elements.size(); k++)
					groups[i].paths[j].elements[k].measurements.death_time_posture_analysis_measure()-=median;
			}
		}
	}
	else{
		//weighted median of all samples of that type
		unsigned long m_pos(0);
		for (unsigned int i = 0; i < groups.size(); i++){
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				if (ns_skip_low_density_paths && groups[i].paths[j].is_low_density_path())
						continue;
				for (unsigned int k = 0; k < groups[i].paths[j].elements.size(); k++)
					groups[i].paths[j].elements[k].measurements.death_time_posture_analysis_measure()-=medians[m_pos];
				m_pos++;
			}
		}
	}
}
void ns_time_path_image_movement_analyzer::generate_movement_description_series(){
	//group sizes and position
	description_series.group_region_sizes.resize(groups.size(),ns_vector_2i(0,0));
	description_series.group_region_position_in_source_image.resize(groups.size(),ns_vector_2i(0,0));
	description_series.group_context_sizes.resize(groups.size(),ns_vector_2i(0,0));
	description_series.group_context_position_in_source_image.resize(groups.size(),ns_vector_2i(0,0));
	for (unsigned int i = 0; i < groups.size(); i++){
		if (groups[i].paths.size()==0)
			continue;
		description_series.group_region_sizes[i] = groups[i].paths[0].path_region_size;
		description_series.group_region_position_in_source_image[i] = groups[i].paths[0].path_region_position;
		description_series.group_context_sizes[i] = groups[i].paths[0].path_context_size;
		description_series.group_context_position_in_source_image[i] = groups[i].paths[0].path_context_position;
	}


	//populate timepoints from solution
	description_series.timepoints.resize(solution->timepoints.size());
	for (unsigned int i = 0; i < solution->timepoints.size(); i++){
		description_series.timepoints[i].time = solution->timepoints[i].time;
		description_series.timepoints[i].worms.reserve(10);
	}
	//add fast moving worms
	for (unsigned int i = 0; i < solution->unassigned_points.stationary_elements.size(); i++){
		description_series.timepoints[solution->unassigned_points.stationary_elements[i].t_id].worms.push_back(
			ns_worm_movement_measurement_description(
				0,
				ns_movement_fast,
				solution->element(solution->unassigned_points.stationary_elements[i]).region_position,
				solution->element(solution->unassigned_points.stationary_elements[i]).region_size,
				-1
				));
	}
	//we go through all paths, assigning path elements to the currect time point in one pass.
	//We keep track of where we are in each path by including the current
	//element id in group_path_current_element_id[].

	for (unsigned int t = 0; t < description_series.timepoints.size(); t++){
		for (unsigned int i = 0; i < groups.size(); i++){
			for (unsigned int j = 0; j < groups[i].paths.size(); j++){
				//see if this path is defined at the provided time point
				const ns_movement_state s(groups[i].paths[j].movement_state(description_series.timepoints[t].time));
				if (s == ns_movement_not_calculated) continue;

				//find the position of the worm at time tt.
				long tt;
				for (tt = 0; tt < (long)groups[i].paths[j].elements.size()-1; tt++){
					if (groups[i].paths[j].elements[tt+1].absolute_time > description_series.timepoints[t].time)
						break;
				}
				
				description_series.timepoints[t].worms.push_back(
					ns_worm_movement_measurement_description(
					0,
					s,
					groups[i].paths[j].elements[tt].region_offset_in_source_image(),
					groups[i].paths[j].elements[tt].worm_region_size(),
					i
					));
			}
		}
	}
}


ns_analyzed_image_time_path_event_index ns_analyzed_image_time_path::find_event_index(const ns_movement_event & event_to_align){
	ns_movement_state_observation_boundary_interval & state_interval(state_intervals[(int)ns_movement_event_state(event_to_align)]);
	if (state_interval.skipped){
		return ns_analyzed_image_time_path_event_index(ns_no_movement_event,ns_movement_state_time_interval_indicies());
	}
	return ns_analyzed_image_time_path_event_index(event_to_align, state_interval.entrance_interval);
}

ns_analyzed_image_time_path_event_index ns_analyzed_image_time_path::find_event_index_with_fallback(const ns_movement_event & event_to_align){
	ns_analyzed_image_time_path_event_index ret(find_event_index(event_to_align));
	//if we've found a result, or can't fallback, return the result
	if (event_to_align ==  ns_fast_movement_cessation || ret.event_type != ns_no_movement_event) return ret;

	//if we don't have the result, fall back one step
	if (event_to_align == ns_translation_cessation)
		return find_event_index(ns_fast_movement_cessation);
	return find_event_index_with_fallback(ns_translation_cessation);
};


void ns_time_path_image_movement_analyzer::generate_death_aligned_movement_posture_visualizations(const bool include_motion_graphs,const unsigned long region_id,const ns_movement_event & event_to_align,const ns_time_path_solution & solution,ns_sql & sql){
	
	ns_64_bit sample_id(0),experiment_id(0);
	string region_name,sample_name,experiment_name;

	ns_region_info_lookup::get_region_info(region_id,&sql,region_name,sample_name,sample_id,experiment_name,experiment_id);
	
	const long thickness(2);

	ns_marker_manager marker_manager;
	
	const ns_vector_2i graph_dimensions(include_motion_graphs?ns_vector_2i(300,200):ns_vector_2i(0,0));
	ns_worm_movement_description_series series = description_series;	

	series.calculate_visualization_grid(graph_dimensions);
	if (series.group_positions_on_visualization_grid.size() != groups.size() ||
		series.group_should_be_displayed.size() != groups.size() ||
		series.group_region_sizes.size() != groups.size())
		throw ns_ex("calculate_visualization_grid() returned an inconsistant result");

	//find time for which each animal needs to be aligned
	vector<ns_analyzed_image_time_path_event_index> path_aligned_event_index(groups.size());
	
	unsigned long alignment_position(0);
	unsigned long aligned_size(0);

	for (unsigned int i = 0; i < groups.size(); i++){
		if (!series.group_should_be_displayed[i])
			path_aligned_event_index[i] = ns_analyzed_image_time_path_event_index(ns_no_movement_event,ns_movement_state_time_interval_indicies(0,0));
		else{
			if (groups[i].paths[0].elements.size() == 0)
				throw ns_ex("ns_time_path_image_movement_analyzer::generate_death_aligned_movement_posture_visualizations()::"
							"Empty path found!");

			path_aligned_event_index[i] = groups[i].paths[0].find_event_index_with_fallback(event_to_align);
			
			//if worms never slow down, align the path to it's final point.
			if (path_aligned_event_index[i].event_type == ns_no_movement_event){
				path_aligned_event_index[i].index.period_end_index = groups[i].paths[0].elements.size()-1;
				path_aligned_event_index[i].event_type = ns_fast_moving_worm_observed;
			}

			if (path_aligned_event_index[i].index.period_end_index > alignment_position)
				alignment_position = path_aligned_event_index[i].index.period_end_index;
		}
	
	}
	//find the last time point in the aligned image series
	for (unsigned int i = 0; i < groups.size(); i++){
		long lp(groups[i].paths[0].elements.size()+alignment_position - path_aligned_event_index[i].index.period_end_index);
		if (lp > aligned_size)
			aligned_size = lp;
	}
	//we now have aligned the different groups.
	//all animals die at index alignment_position
	//and there are latest_aligned_index frames in the video.
	
	vector<ns_image_storage_source_handle<ns_8_bit> > path_image_source;
	vector<ns_graph> path_movement_graphs(groups.size());
	path_image_source.reserve(groups.size());

	ns_image_standard output_image;
	ns_image_properties prop;
	prop.components = 3;
	prop.width = series.visualization_grid_dimensions.x;
	prop.height = series.visualization_grid_dimensions.y;
	output_image.init(prop);

	
	ns_image_properties graph_prop(prop);
	ns_image_standard graph_temp;
	if (include_motion_graphs){
		graph_prop.width = series.metadata_dimensions.x;
		graph_prop.height = series.metadata_dimensions.y;
		graph_temp.init(graph_prop);
	}
	else
		graph_prop.width = graph_prop.height = 0;

	//initialize path movement image source for first frame
	for (unsigned int i = 0; i < groups.size(); i++){
		if (!series.group_should_be_displayed[i]){
			path_image_source.push_back(ns_image_storage_source_handle<ns_8_bit>(0)); //we want the index of groups[i] and path_image_source[i] to match so we fill unwanted groups with a dummy entry.
			continue;
		}
		groups[i].paths[0].output_image.load_from_db(groups[i].paths[0].output_image.id,&sql);
		path_image_source.push_back(image_server.image_storage.request_from_storage(groups[i].paths[0].output_image,&sql));
		groups[i].paths[0].initialize_movement_image_loading(path_image_source[i]);
	}

	//make movement graphs
	if (include_motion_graphs){
		for (unsigned int i = 0; i < groups.size(); i++){
			if (!series.group_should_be_displayed[i]) continue;
			ns_make_path_movement_graph(groups[i].paths[0],path_movement_graphs[i]);
			path_movement_graphs[i].draw(graph_temp);
		}
	}

	//load first frame of all paths
	/*for (unsigned int g = 0; g < groups.size(); g++){
		if (!series.group_should_be_displayed[g]) continue;
		groups[g].paths[0].load_movement_images(ns_analyzed_time_image_chunk(0,1),path_image_source[g]);
	}*/

	//now go through each measurement time for the solution
	for (unsigned long t = 0; t < aligned_size; t++){
		cerr << (100*t)/aligned_size << "%...";

		ns_movement_posture_visualization_summary vis_summary;
		vis_summary.region_id = region_id;
		vis_summary.frame_number = t;
		vis_summary.alignment_frame_number = alignment_position;

		//init output image
		for (unsigned long y = 0; y < prop.height; y++)
			for (unsigned long x = 0; x < 3*prop.width; x++)
				output_image[y][x] = 0;

		bool image_to_output(false);

		//go through each path
		for (unsigned int g = 0; g < groups.size(); g++){
			const unsigned long path_id(0);
			ns_analyzed_image_time_path & path(groups[g].paths[path_id]);

			long i((long)t-((long)alignment_position - (long)path_aligned_event_index[g].index.period_end_index));
			
			if (!series.group_should_be_displayed[g] || i >= (long)path.elements.size()) 
				continue;

	
			
			ns_color_8 c;
			if (i > path.elements.size()){
				c = ns_movement_colors::color(ns_movement_not_calculated);
				if (include_motion_graphs)marker_manager.set_marker(-1,path_movement_graphs[g]);
			}
			else if (i >= 0){
				c = ns_movement_colors::color(path.movement_state(path.elements[i].absolute_time));
				if (include_motion_graphs)marker_manager.set_marker(path.elements[i].absolute_time,path_movement_graphs[g]);
			}
			else{
				c = ns_movement_colors::color(ns_movement_fast)/2;
				if (include_motion_graphs)marker_manager.set_marker(-1,path_movement_graphs[g]);
			}

			if (include_motion_graphs)path_movement_graphs[g].draw(graph_temp);
			

			
			int scale=1;
			if (i < 0)
				scale = 2;

			if (include_motion_graphs){
				for (unsigned int y = 0; y < graph_prop.height; y++){
					for (unsigned int x = 0; x < 3*graph_prop.width; x++){
						output_image[y+series.metadata_positions_on_visualization_grid[g].y]
						[x+ 3*series.metadata_positions_on_visualization_grid[g].x] = graph_temp[y][x]/scale;
					}
				}
			}

		
			//draw colored line around worm
			const ns_vector_2i & p (series.group_positions_on_visualization_grid[g]);
			const ns_vector_2i & s (series.group_context_sizes[g]);

			ns_vector_2i pos(series.group_positions_on_visualization_grid[g]);
			if (include_motion_graphs){
				if ( series.group_context_sizes[g].x < graph_temp.properties().width)
					pos.x += (graph_temp.properties().width-series.group_context_sizes[g].x)/2;
			}


		//	const ns_vector_2i s (series.metadata_positions_on_visualization_grid[g]+ns_vector_2i(graph_prop.width+thickness,graph_prop.height));
			output_image.draw_line_color_thick(pos+ns_vector_2i(-thickness,-thickness),pos+ns_vector_2i(s.x,-thickness),c,thickness);
			output_image.draw_line_color_thick(pos+ns_vector_2i(-thickness,-thickness),pos+ns_vector_2i(-thickness,s.y),c,thickness);
			output_image.draw_line_color_thick(pos+s,pos+ns_vector_2i(s.x,-thickness),c,thickness);
			output_image.draw_line_color_thick(pos+s,pos+ns_vector_2i(-thickness,s.y),c,thickness);
			
			if (i >= 0){
				const unsigned long time(path.elements[i].absolute_time);
				//find most up to date frame for the path;
				ns_analyzed_time_image_chunk chunk(i,i+1);
				path.load_movement_images(chunk,path_image_source[g]);
				if (i > 0)
					path.elements[i-1].clear_movement_images(); 

				image_to_output = true;
				//transfer over movement visualzation to the correct place on the grid.
				ns_image_standard im;
				path.elements[i].generate_movement_visualization(im);
		
				if (im.properties().height == 0)
					throw ns_ex("Registered images not loaded! Path ") << g << " i " << i;

				string::size_type ps(vis_summary.worms.size());
				vis_summary.worms.resize(ps+1);

				vis_summary.worms[ps].path_in_source_image.position = path.path_region_position;
				vis_summary.worms[ps].path_in_source_image.size= path.path_region_size;

				vis_summary.worms[ps].worm_in_source_image.position = path.elements[i].region_offset_in_source_image();
				vis_summary.worms[ps].worm_in_source_image.size = path.elements[i].worm_region_size();

				vis_summary.worms[ps].path_in_visualization.position = pos;
				vis_summary.worms[ps].path_in_visualization.size=  series.group_context_sizes[g];

				if (include_motion_graphs){
					vis_summary.worms[ps].metadata_in_visualizationA.position = series.metadata_positions_on_visualization_grid[g];
					vis_summary.worms[ps].metadata_in_visualizationA.size = ns_vector_2i(graph_prop.width,graph_prop.height);
				}
				else vis_summary.worms[ps].metadata_in_visualizationA.position = vis_summary.worms[ps].metadata_in_visualizationA.size = ns_vector_2i(0,0);

				vis_summary.worms[ps].stationary_path_id.path_id = path_id;
				vis_summary.worms[ps].stationary_path_id.group_id = g;
				vis_summary.worms[ps].stationary_path_id.detection_set_id = analysis_id;
				vis_summary.worms[ps].path_time.period_end = path.elements.rbegin()->absolute_time;
				vis_summary.worms[ps].path_time.period_start = path.elements[0].absolute_time;
				vis_summary.worms[ps].image_time = path.elements[i].absolute_time;

			
				//ns_vector_2i region_offset(path.elements[i].region_offset_in_source_image()-path.elements[i].context_offset_in_source_image());
				for (unsigned int y = 0; y < vis_summary.worms[ps].path_in_visualization.size.y; y++){
					for (unsigned int x = 0; x < 3*vis_summary.worms[ps].path_in_visualization.size.x; x++){
						output_image[y+pos.y]
						[x+ 3*pos.x] = im[y][x];
					}
				}
			}


		}
		string metadata;
		vis_summary.to_xml(metadata);
		output_image.set_description(metadata);
		ns_image_server_captured_image_region reg;
		reg.region_images_id = 0;
		reg.region_info_id = region_id;
		reg.region_name = region_name;
		reg.sample_id = sample_id;
		reg.experiment_id = experiment_id;
		reg.sample_name = sample_name;
		reg.experiment_name = experiment_name;
		ns_image_server_image im(reg.create_storage_for_aligned_path_image(t,(unsigned long)event_to_align,ns_tiff,sql,ns_movement_event_to_label(event_to_align)));
		try{
			bool had_to_use_volatile_storage;
			ns_image_storage_reciever_handle<ns_8_bit> r(image_server.image_storage.request_storage(im,ns_tiff,1024,&sql,had_to_use_volatile_storage,false,false));
			output_image.pump(r.output_stream(),1024);
			im.mark_as_finished_processing(&sql);
		}
		catch(ns_ex & ex){
			im.mark_as_finished_processing(&sql);
			throw ex;
		}
	}
}

void ns_time_path_image_movement_analyzer::generate_movement_posture_visualizations(const bool include_graphs,const unsigned long region_id,const ns_time_path_solution & solution,ns_sql & sql){
	const unsigned long thickness(3);

	ns_marker_manager marker_manager;

	const ns_vector_2i graph_dimensions(include_graphs?ns_vector_2i(300,200):ns_vector_2i(0,0));
	ns_worm_movement_description_series series = description_series;
	series.calculate_visualization_grid(graph_dimensions);
	if (series.group_positions_on_visualization_grid.size() != groups.size() ||
		series.group_should_be_displayed.size() != groups.size() ||
		series.group_region_sizes.size() != groups.size())
		throw ns_ex("calculate_visualization_grid() returned an inconsistant result");

	vector<unsigned long> path_i(groups.size(),0);
	vector<ns_image_storage_source_handle<ns_8_bit> > path_image_source;
	vector<ns_graph> path_movement_graphs(groups.size());
	path_image_source.reserve(groups.size());

	ns_image_standard output_image;
	ns_image_properties prop;
	prop.components = 3;
	prop.width = series.visualization_grid_dimensions.x;
	prop.height = series.visualization_grid_dimensions.y;
	output_image.init(prop);

	ns_image_properties graph_prop(prop);
	ns_image_standard graph_temp;
	if (include_graphs){
		graph_prop.width = series.metadata_dimensions.x;
		graph_prop.height = series.metadata_dimensions.y;
		graph_temp.init(graph_prop);
	}
	else graph_prop.width = graph_prop.height = 0;

	//initialize path movement image source for first frame
	for (unsigned int i = 0; i < groups.size(); i++){
		if (!series.group_should_be_displayed[i]){
			path_image_source.push_back(ns_image_storage_source_handle<ns_8_bit>(0)); //we want the index of groups[i] and path_image_source[i] to match so we fill unwanted groups with a dummy entry.
			continue;
		}
		groups[i].paths[0].output_image.load_from_db(groups[i].paths[0].output_image.id,&sql);
		path_image_source.push_back(image_server.image_storage.request_from_storage(groups[i].paths[0].output_image,&sql));
		groups[i].paths[0].initialize_movement_image_loading(path_image_source[i]);
	}

	//make movement 
	if (include_graphs){
		for (unsigned int i = 0; i < groups.size(); i++){
			if (!series.group_should_be_displayed[i]) continue;
			ns_make_path_movement_graph(groups[i].paths[0],path_movement_graphs[i]);
			path_movement_graphs[i].draw(graph_temp);
		}
	}

	//load first frame of all paths
	for (unsigned int g = 0; g < groups.size(); g++){
		if (!series.group_should_be_displayed[g]) continue;
		groups[g].paths[0].load_movement_images(ns_analyzed_time_image_chunk(0,1),path_image_source[g]);
	}
	//now go through each measurement time for the solution
	for (unsigned long t = 0; t < solution.timepoints.size(); t++){
		cerr << (100*t)/solution.timepoints.size() << "%...";

		ns_movement_posture_visualization_summary vis_summary;
		vis_summary.region_id = region_id;


		//init output image
		for (unsigned long y = 0; y < prop.height; y++)
			for (unsigned long x = 0; x < 3*prop.width; x++)
				output_image[y][x] = 0;
		bool image_to_output(false);

		const unsigned long time(solution.timepoints[t].time);
		//go through each path
		for (unsigned int g = 0; g < groups.size(); g++){
			if (!series.group_should_be_displayed[g]) continue;
			const unsigned long path_id(0);
			ns_analyzed_image_time_path & path(groups[g].paths[path_id]);
			unsigned long & i(path_i[g]);
			if (i >= path.elements.size()) continue;
			//find most up to date frame for the path;
			unsigned best_frame;

			for (best_frame = i; best_frame < path.elements.size() && path.elements[best_frame].absolute_time < time; best_frame++);
			if (best_frame == path.elements.size())
				continue;
			if (best_frame == 0 && path.elements[best_frame].absolute_time > time)
				continue;
			if (path.elements[best_frame].absolute_time > time)
				best_frame--;

			//now load frames until done
			if (best_frame > i+1)
				cerr << "Skipping" << best_frame - i << " frames from path " << g << "\n";

			for (; i < best_frame;){
			//	cerr << "Clearing p"<<g<< " " << i << "\n";
				if (i!=0)
					path.elements[i-1].clear_movement_images(); 
				++i;
				ns_analyzed_time_image_chunk chunk(i,i+1);
			//	cerr << "loading p"<<g<< " " << i << "\n";
				path.load_movement_images(chunk,path_image_source[g]);
			}

			image_to_output = true;
			//transfer over movement visualzation to the correct place on the grid.
			ns_image_standard im;
			ns_color_8 c;
			path.elements[i].generate_movement_visualization(im);
			c = ns_movement_colors::color(path.movement_state(time));
		
			if (im.properties().height == 0)
				throw ns_ex("Registered images not loaded! Path ") << g << " i " << i;


			string::size_type ps(vis_summary.worms.size());
			vis_summary.worms.resize(ps+1);
			vis_summary.worms[ps].path_in_source_image.position = path.path_region_position;
			vis_summary.worms[ps].path_in_source_image.size= path.path_region_size;

			vis_summary.worms[ps].worm_in_source_image.position = path.elements[i].region_offset_in_source_image();
			vis_summary.worms[ps].worm_in_source_image.size = path.elements[i].worm_region_size();

			vis_summary.worms[ps].path_in_visualization.position = series.group_positions_on_visualization_grid[g];
			vis_summary.worms[ps].path_in_visualization.size= series.group_context_sizes[g];

			if (include_graphs){
				vis_summary.worms[ps].metadata_in_visualizationA.position = series.metadata_positions_on_visualization_grid[g];
				vis_summary.worms[ps].metadata_in_visualizationA.size = ns_vector_2i(graph_prop.width,graph_prop.height);
			}
			else vis_summary.worms[ps].metadata_in_visualizationA.position = vis_summary.worms[ps].metadata_in_visualizationA.size = ns_vector_2i(0,0);

			vis_summary.worms[ps].stationary_path_id.path_id = path_id;
			vis_summary.worms[ps].stationary_path_id.group_id = g;
			vis_summary.worms[ps].stationary_path_id.detection_set_id = analysis_id;
			vis_summary.worms[ps].path_time.period_start = path.elements[i].absolute_time;
			vis_summary.worms[ps].path_time.period_end = path.elements.rbegin()->absolute_time;
			vis_summary.worms[ps].image_time = path.elements[i].absolute_time;

			
			//ns_vector_2i region_offset(path.elements[i].region_offset_in_source_image()-path.elements[i].context_offset_in_source_image());
			for (unsigned int y = 0; y < vis_summary.worms[ps].path_in_visualization.size.y; y++){
				for (unsigned int x = 0; x < 3*vis_summary.worms[ps].path_in_visualization.size.x; x++){
					output_image[y+vis_summary.worms[ps].path_in_visualization.position.y]
					[x+ 3*vis_summary.worms[ps].path_in_visualization.position.x] = im[y/*+region_offset.y*/][x/*+3*region_offset.x*/];
				}
			}

			if (include_graphs){
				marker_manager.set_marker(time,path_movement_graphs[g]);
				path_movement_graphs[g].draw(graph_temp);

				for (unsigned int y = 0; y < vis_summary.worms[ps].metadata_in_visualizationA.size.y; y++){
					for (unsigned int x = 0; x < 3*vis_summary.worms[ps].metadata_in_visualizationA.size.x; x++){
						output_image[y+vis_summary.worms[ps].metadata_in_visualizationA.position.y]
						[x+ 3*vis_summary.worms[ps].metadata_in_visualizationA.position.x] = graph_temp[y][x];
					}
				}
			}


			//draw colored line around worm
			const ns_vector_2i & p (vis_summary.worms[ps].path_in_visualization.position);
			const ns_vector_2i & s (vis_summary.worms[ps].path_in_visualization.size);
			output_image.draw_line_color_thick(p,p+ns_vector_2i(s.x,0),c,thickness);
			output_image.draw_line_color_thick(p,p+ns_vector_2i(0,s.y),c,thickness);
			output_image.draw_line_color_thick(p+s,p+ns_vector_2i(s.x,0),c,thickness);
			output_image.draw_line_color_thick(p+s,p+ns_vector_2i(0,s.y),c,thickness);
		}
		string metadata;
		vis_summary.to_xml(metadata);
		output_image.set_description(metadata);
		ns_image_server_captured_image_region reg;
		reg.region_images_id = solution.timepoints[t].sample_region_image_id;
		ns_image_server_image im(reg.create_storage_for_processed_image(ns_process_movement_posture_visualization,ns_tiff,&sql));
		try{
			bool had_to_use_volatile_storage;
			ns_image_storage_reciever_handle<ns_8_bit> r(image_server.image_storage.request_storage(im,ns_tiff,1024,&sql,had_to_use_volatile_storage,false,false));
			output_image.pump(r.output_stream(),1024);
			im.mark_as_finished_processing(&sql);
		}
		catch(ns_ex & ex){
			im.mark_as_finished_processing(&sql);
			throw ex;
		}
	}
};


//we step through all region visualizations between start_i and stop_i, ordered by time in region_image_specifications
//and populate any worm images that are needed from each region image.
void ns_time_path_image_movement_analyzer::load_region_visualization_images(const unsigned long start_i, const unsigned long stop_i,const unsigned int start_group, const unsigned int stop_group,ns_sql & sql, bool just_do_a_consistancy_check){
	if (stop_group > groups.size())
		throw ns_ex("ns_time_path_image_movement_analyzer::load_region_visualization_images()::An invalid group was specified: ") << stop_group << " (there are only " << groups.size() << " groups)";
//	cerr << "Looking for region images from time point " << start_i << " until " << stop_i;
	bool problem_occurred(false);
	ns_ex problem;
	for (unsigned long i = start_i; i < stop_i; i++){
		//check to see if the region images are needed at this time point
		region_image_specifications[i].region_vis_required = false;
		region_image_specifications[i].interpolated_region_vis_required = false;
		for (unsigned int g = start_group; g < stop_group; g++){
			
			#ifdef NS_OUTPUT_ALGINMENT_DEBUG
			//xxx
			if (g !=1)
				continue;
			#endif
			for (unsigned int p = 0; p < groups[g].paths.size(); p++){

				bool region_vis_required = groups[g].paths[p].region_image_is_required(region_image_specifications[i].time,false,false);
				bool interpolated_region_vis_required = groups[g].paths[p].region_image_is_required(region_image_specifications[i].time,true,false);
				
				region_image_specifications[i].region_vis_required = 
					region_image_specifications[i].region_vis_required || region_vis_required;
				region_image_specifications[i].interpolated_region_vis_required = 
					region_image_specifications[i].interpolated_region_vis_required || interpolated_region_vis_required;
			}
		}
		if (!region_image_specifications[i].region_vis_required && !region_image_specifications[i].interpolated_region_vis_required)
			continue;
		try{
			//region images contain the context images
			if (region_image_specifications[i].region_vis_required){
				
				ns_image_storage_source_handle<ns_8_bit> in(image_server.image_storage.request_from_storage(region_image_specifications[i].region_vis_image,&sql));
				image_loading_temp.use_more_memory_to_avoid_reallocations();
				in.input_stream().pump(image_loading_temp,1024);
			}
			else image_loading_temp.prepare_to_recieve_image(ns_image_properties(0,0,0,0));

			if (region_image_specifications[i].interpolated_region_vis_image.id != 0){
				image_loading_temp2.use_more_memory_to_avoid_reallocations();
				ns_image_storage_source_handle<ns_8_bit> in2(image_server.image_storage.request_from_storage(region_image_specifications[i].interpolated_region_vis_image,&sql));
				in2.input_stream().pump(image_loading_temp2,1024);
			}
			else image_loading_temp2.prepare_to_recieve_image(ns_image_properties(0,0,0,0));

	//		cerr << "Done.\n";
			//extract green channel, as this has the grayscale values.
			ns_image_properties prop(image_loading_temp.properties());
			if (!region_image_specifications[i].region_vis_required)
				prop = image_loading_temp2.properties();
			if (prop.components != 3)
				throw ns_ex("ns_time_path_image_movement_analyzer::load_region_visualization_images()::Region images must be RBG");
			bool new_data_allocated = false;
			for (unsigned int g = start_group; g < stop_group; g++){
			
				for (unsigned int p = 0; p < groups[g].paths.size(); p++){
					if (g >= groups.size())
						throw ns_ex("ns_time_path_image_movement_analyzer::load_region_visualization_images()::An invalid group was specified: ") << g << " (there are only " << groups.size() << " groups)";
	//
					if (p >= groups[g].paths.size())
						throw ns_ex("ns_time_path_image_movement_analyzer::load_region_visualization_images()::An invalid path was specified: ") << p << " (there are only " << groups[g].paths.size() << " paths)";
			//
		//			ns_movement_image_collage_info m(groups[g].paths[p]);
					if (groups[g].paths[p].populate_images_from_region_visualization(region_image_specifications[i].time,image_loading_temp,image_loading_temp2,just_do_a_consistancy_check))
						new_data_allocated = true;
				}
			}
			if (!just_do_a_consistancy_check && new_data_allocated){
				memory_pool.aligned_image_pool.mark_stack_size_waypoint_and_trim();
				memory_pool.registered_image_pool.mark_stack_size_waypoint_and_trim();
			}
		}
		catch(ns_ex & ex){
			problem_occurred = true;
			problem = ex;
			ns_64_bit problem_id = image_server.register_server_event(ns_image_server::ns_register_in_central_db,ns_image_server_event(ex.text()));
			sql << " UPDATE sample_region_images SET problem = " << problem_id << " where id = " << region_image_specifications[i].sample_region_image_id;
			sql.send_query();
		}
	}
	if (problem_occurred)
		throw problem;
}

#include "ns_threshold_movement_posture_analyzer.h"
#include "ns_hidden_markov_model_posture_analyzer.h"
ns_time_path_posture_movement_solution ns_threshold_movement_posture_analyzer::operator() (ns_analyzed_image_time_path * path, const bool fill_in_loglikelihood_timeseries,std::ostream * debug_output)const{
	return run(path,(std::ostream * )debug_output);
}
ns_time_path_posture_movement_solution ns_threshold_movement_posture_analyzer::operator()(const ns_analyzed_image_time_path * path, std::ostream * debug_output) const{
	return run(path,debug_output);
}
unsigned long ns_threshold_movement_posture_analyzer::latest_possible_death_time(const ns_analyzed_image_time_path * path,const unsigned long last_observation_time) const{
	return  last_observation_time - parameters.permanance_time_required_in_seconds;
}
ns_time_path_posture_movement_solution ns_threshold_movement_posture_analyzer::run(const ns_analyzed_image_time_path * path, std::ostream * debug_output) const{

	
	bool found_slow_movement_cessation(false),
			found_posture_change_cessation(false);


	//since the movement images are calculated as image[t]-image[t-1], that means the movement occurred between time points t and t-1.
	//thus if the movement ratio drops below a threshold at time t, the animal died between time t and t-1.
	//thus when an animal's movement ratio drops below some threshold at time t, we annotate it's death at time t.

	//an exception is made for animals whose movement ratio drops below a threshold at the second frame.
	//this is because we can't calculate reasonable movement ratios in the first frame, and thus assume the animals 
	//died the frame before it.
	long last_valid_event_index(-1),
		first_valid_event_index(path->element_count());

	
	for (long t = 0; t < path->element_count(); t++){
		if (path->element(t).excluded || path->element(t).censored || path->element(t).element_before_fast_movement_cessation) continue;
		if (first_valid_event_index == path->element_count()){
			first_valid_event_index = t;
		}
		last_valid_event_index = t;
	}

		
	if (last_valid_event_index == -1){
		/*ns_time_path_posture_movement_solution sol;
		sol.dead.skipped = true;
		sol.moving.skipped = true;
		sol.slowing.skipped = true;
		return sol;*/
		throw ns_ex("Encountered a path without valid elements!");
	}
	long last_time_point_at_which_slow_movement_was_observed(first_valid_event_index),
			last_time_point_at_which_posture_changes_were_observed(first_valid_event_index);

	unsigned long longest_gap_in_slow(0),longest_gap_in_posture(0),longest_gap_in_dead(0);

	
	for (long t = first_valid_event_index ; t < path->element_count(); t++){
		if (path->element(t).excluded || path->element(t).censored || path->element(t).element_before_fast_movement_cessation) continue;
		double r(path->element(t).measurements.death_time_posture_analysis_measure());

		unsigned long observation_gap(0);
		if (t > first_valid_event_index){
			observation_gap = path->element(t).absolute_time - path->element(t-1).absolute_time;
		}
		


		const unsigned long &cur_time (path->element(t).absolute_time);
		//keep on pushing the last posture time and last sationary
		//times forward until we hit a low enough movement ratio
		//to meet the criteria.  At that point, the last posture 
		//and last stationary cutoffs stick
		
		
		if (r >= parameters.posture_cutoff && !found_slow_movement_cessation)
			last_time_point_at_which_slow_movement_was_observed = t;
		if (r >=  parameters.stationary_cutoff&& !found_posture_change_cessation)
			last_time_point_at_which_posture_changes_were_observed = t;

		if (!found_slow_movement_cessation && longest_gap_in_slow < observation_gap)
			longest_gap_in_slow = observation_gap;
		
		if (!found_posture_change_cessation && longest_gap_in_posture < observation_gap)  //gaps in slow movement count towards gaps in posture change.
			longest_gap_in_posture = observation_gap;
		
		if (found_posture_change_cessation && longest_gap_in_dead < observation_gap)  //gaps in slow movement count towards gaps in posture change.
			longest_gap_in_dead = observation_gap;
		

		if (!found_slow_movement_cessation &&

			//if the last time the worm was moving slowly enough to count 
			//as changing posture or remaining stationary is a long time ago
			//mark that time as the change.
			(cur_time - path->element(last_time_point_at_which_slow_movement_was_observed).absolute_time) >= parameters.permanance_time_required_in_seconds){
				//posture_start_time = elements[last_posture_index+1].absolute_time;
				found_slow_movement_cessation = true;
				//since we can't make accurate measurements of the first time point (we can't measure movement
				//as we don't have a prior measurement of it if an event falls one after it,
				//shift it all the way to the first time point.
				if (last_time_point_at_which_slow_movement_was_observed == 1 && first_valid_event_index == 0)
					last_time_point_at_which_slow_movement_was_observed = 0;
			
		}

		if (!found_posture_change_cessation &&

			(cur_time - path->element(last_time_point_at_which_posture_changes_were_observed).absolute_time) > parameters.permanance_time_required_in_seconds){
				//stationary_start_time = elements[last_stationary_index+1].absolute_time;
				found_posture_change_cessation = true;
				if (last_time_point_at_which_posture_changes_were_observed == 1 && first_valid_event_index == 0)
					last_time_point_at_which_posture_changes_were_observed = 0;
			}
	}

	ns_time_path_posture_movement_solution solution;
	//assumed to start as slow, so it's only skipped if it is skipped over

	solution.dead.longest_observation_gap_within_interval = longest_gap_in_dead;
	solution.slowing.longest_observation_gap_within_interval = longest_gap_in_posture;
	solution.moving.longest_observation_gap_within_interval = longest_gap_in_slow;

	solution.moving.skipped = last_time_point_at_which_slow_movement_was_observed == first_valid_event_index  && (found_slow_movement_cessation || found_posture_change_cessation);
	if (!solution.moving.skipped){
		solution.moving.start_index = first_valid_event_index ;
		solution.moving.end_index = last_time_point_at_which_slow_movement_was_observed;
		if (!found_slow_movement_cessation && !found_posture_change_cessation)
			solution.moving.end_index = last_valid_event_index;
	//	solution.moving.end_index = 0;
	}

	solution.slowing.skipped = !found_slow_movement_cessation	//if the animal never slowed down to changing posture
		
								|| found_posture_change_cessation &&   //or it was skipped over as the animal went straight to death
								last_time_point_at_which_posture_changes_were_observed == last_time_point_at_which_slow_movement_was_observed;
	if (!solution.slowing.skipped){
		solution.slowing.start_index = (!solution.moving.skipped)?(last_time_point_at_which_slow_movement_was_observed):first_valid_event_index;
		solution.slowing.end_index = last_time_point_at_which_posture_changes_were_observed;
		if (!found_posture_change_cessation)
			solution.slowing.end_index = last_valid_event_index;
	}
							//death can't be skipped over, so it's only skipped if it is never observed to occur
	solution.dead.skipped = !found_posture_change_cessation;
	if (!solution.dead.skipped){
		/*if (solution.moving.skipped && solution.slowing.skipped)
			solution.dead.start_index = 0;
		else */
		solution.dead.start_index = last_time_point_at_which_posture_changes_were_observed;
		//else if (solution.slowing.skipped)
		//	solution.dead.start_index = solution.moving.end_index;
		//else solution.dead.start_index = solution.slowing.end_index;
		solution.dead.end_index = last_valid_event_index;
	}
	if (solution.dead.skipped &&
		solution.slowing.skipped &&
		solution.moving.skipped)
		throw ns_ex("Producing an all-skipped estimate!");
	if (!solution.moving.skipped && solution.moving.start_index == solution.moving.end_index)
		throw ns_ex("Unskipped 0-measurement movement period!");
	if (!solution.slowing.skipped && solution.slowing.start_index == solution.slowing.end_index)
		throw ns_ex("Unskipped 0-measurement slowing period");
	if (!solution.dead.skipped && solution.dead.start_index == solution.dead.end_index)
		throw ns_ex("Unskipped 0-measurement dead period");
	if (!solution.slowing.skipped && !solution.dead.skipped && solution.dead.start_index > solution.slowing.end_index)
		throw ns_ex("Reversed death and slow movement cessation times!");
	solution.loglikelihood_of_solution = 0;
	return solution;
}



ns_analyzed_image_time_path_death_time_estimator * ns_get_death_time_estimator_from_posture_analysis_model(const ns_posture_analysis_model & m){
	if (m.posture_analysis_method == ns_posture_analysis_model::ns_hidden_markov){
		return new ns_time_path_movement_markov_solver(m.hmm_posture_estimator);
	}
	else if (m.posture_analysis_method == ns_posture_analysis_model::ns_threshold){
		return new ns_threshold_movement_posture_analyzer(m.threshold_parameters);
	}
	else if (m.posture_analysis_method == ns_posture_analysis_model::ns_not_specified)
		throw ns_ex("ns_get_death_time_estimator_from_posture_analysis_model()::No posture analysis method specified.");
	throw ns_ex("ns_get_death_time_estimator_from_posture_analysis_model()::Unknown posture analysis method!");
}
