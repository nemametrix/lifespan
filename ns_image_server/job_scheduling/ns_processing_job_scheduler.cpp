#include "ns_processing_job_scheduler.h"
#include "ns_image_processing_pipeline.h"
#include "ns_processing_job_processor.h"

using namespace std;


struct ns_image_processing_pipline_holder{
public:
	ns_image_processing_pipline_holder(const unsigned long pipeline_chunk_size):pipeline(pipeline_chunk_size){}
	ns_image_processing_pipeline pipeline;
};

std::string ns_maintenance_task_to_string(const ns_maintenance_task & task){
	switch(task){
		case ns_maintenance_rebuild_sample_time_table:
			return "Rebuild sample times table";
		case ns_maintenance_rebuild_worm_movement_table:
			return "Rebuild worm movement table";
		case ns_maintenance_update_processing_job_queue:
			return "Add new jobs to queue";
		case ns_maintenance_rebuild_movement_data:
			return "Build Movement Data";
		case ns_maintenance_rebuild_movement_from_stored_image_quantification:
			return "Rebuild Movement Data From Stored Image Quantification";
		case ns_maintenance_rebuild_movement_from_stored_images:
			return "Rebuild Movement Data From Stored Images";
		case ns_maintenance_generate_movement_posture_visualization:
			return "Generate Movement Posture Visualization";
		case ns_maintenance_recalculate_censoring:
			return "Recalculate Censoring";
		case ns_maintenance_generate_movement_posture_aligned_visualization:
			return "Generate Movement Posture Aligned Visualization";
		case ns_maintenance_generate_sample_regions_from_mask:
			return "Generate Sample Regions from Mask";
		case ns_maintenance_delete_images_from_database:
			return "Delete Images from the Database";
		case ns_maintenance_delete_files_from_disk_request:
			return "Submit Request for files to be delelted from disk";
		case ns_maintenance_delete_files_from_disk_action:
			return "Delete files from disk";
		case ns_maintenance_check_for_file_errors:
			return "Check for missing or broken files";
		case ns_maintenance_determine_disk_usage:
			return "Determine disk usage for files";
		case ns_maintenance_generate_animal_storyboard:
			return "Generate Animal Storyboard";
		case ns_maintenance_generate_animal_storyboard_subimage:
			return "Process Animal Storyboard Sub-Image";
		case ns_maintenance_compress_stored_images:
			return "Compress Stored Images";
		case ns_maintenance_generate_subregion_mask:
			return "Generate Subregion Mask";
		case ns_maintenance_rerun_image_registration:
			return "Re-Run Image Registration";
		case ns_maintenance_recalc_image_stats:
			return "Recalculate Image Statistics";
		case ns_maintenance_last_task: throw ns_ex("ns_maintenance_task_to_string::last_task does not have a std::string representation");
		default: 
			throw ns_ex("ns_maintenance_task_to_string::Unknown Maintenance task");
	}
}

bool ns_processing_job_scheduler::run_a_job(ns_sql & sql,bool first_in_first_out_job_queue){
	
	//XXX
	/*ns_image_server_captured_image im;
	im.captured_images_id =  5595918;
	std::vector<ns_image_server_captured_image_region> reg_im;
	pipeline->pipeline.apply_mask(im,reg_im,sql);
	exit(0);*/
	//XXXX

	//if we can't talk to the long term storage we're bound to fail, so don't try.
	image_server.image_storage.test_connection_to_long_term_storage(true);
	if (!image_server.image_storage.long_term_storage_was_recently_writeable())
		return false;
	ns_image_server_push_job_scheduler push_scheduler;

	ns_processing_job job = push_scheduler.request_job(sql,first_in_first_out_job_queue);
	
	if (job.id == 0)
		return false;
	
	//refresh flag labels from db
	ns_death_time_annotation_flag::get_flags_from_db(sql);

	if (job.maintenance_task == ns_maintenance_update_processing_job_queue){
		image_server.register_server_event(ns_image_server_event("Updating job queue"),&sql);

		sql << "DELETE from processing_jobs WHERE maintenance_task =" << ns_maintenance_update_processing_job_queue;
		sql.send_query();
		push_scheduler.report_job_as_finished(job,sql);
		push_scheduler.discover_new_jobs(sql);
		return true;		
	}

	ns_acquire_for_scope<ns_processing_job_processor> processor(
		ns_processing_job_processor_factory::generate(job,image_server,this->pipeline->pipeline));
		
	try{
		std::string rejection_reason;
		if (!processor().job_is_still_relevant(sql,rejection_reason)){
			image_server.register_server_event(ns_image_server_event("Encountered a processing job queue that had already been performed or invalidated: ") << rejection_reason << "[" << job.description() << "]",&sql);
			push_scheduler.report_job_as_finished(job,sql);
			if (processor().delete_job_after_processing())
				processor().delete_job(sql);
			processor.release();
			sql.send_query("COMMIT");
			return true;
		}
		if(idle_timer_running)
			image_server.performance_statistics.register_job_duration(ns_performance_statistics_analyzer::ns_idle,idle_timer.stop());
		idle_timer_running = false;
		ns_high_precision_timer tp;
		tp.start();

		//mark the subject as busy to prevent multiple jobs running simultaneously on the same data
		processor().mark_subject_as_busy(true,sql);
		sql.send_query("COMMIT");

		//update UI to show job is being performed, if requested.
		if (processor().flag_job_as_being_processed_before_processing())
			processor().flag_job_as_being_processed(sql);
		sql.send_query("COMMIT");

		if (processor().run_job(sql))
		processor().mark_subject_as_busy(false,sql);

		push_scheduler.report_job_as_finished(job,sql);
		sql.send_query("COMMIT");

		processor().handle_concequences_of_job_completion(sql);

		if (processor().delete_job_after_processing())
			processor().delete_job(sql);
		sql.send_query("COMMIT");

		processor.release();
		image_server.performance_statistics.register_job_duration(ns_performance_statistics_analyzer::ns_running_a_job,tp.stop());
		
		idle_timer_running = true;
		idle_timer.start();

		return true;
	}
	catch(ns_ex & ex){
		//we have found an error, handle it by registering it in the
		//host_event log, and annotate the current job (and any associated images)
		//with a reference to the error that occurred.
		sql.clear_query();
		
		processor().mark_subject_as_busy(false,sql);


		ns_64_bit error_id(push_scheduler.report_job_as_problem(job,ex,sql));
		sql.send_query("COMMIT");

		//there are a variety of problems that could cause an exception to be thrown.
		//only mark the image itself as problematic if the error doesn't come from 
		//any of the environmental problems that can crop up.
		bool problem_with_long_term_storage(!image_server.image_storage.long_term_storage_was_recently_writeable());
		if (!problem_with_long_term_storage &&
			ex.type() != ns_network_io && 
			ex.type() != ns_sql_fatal &&  
			ex.type() != ns_memory_allocation && 
			ex.type() != ns_cache)
			processor().mark_subject_as_problem(error_id,sql);

		image_server.performance_statistics.cancel_outstanding_jobs();
		if (ex.type() == ns_memory_allocation)
			throw;	//memory allocation errors can cause big, long-term problems, thus we need to pass
					//them downwards to be handled.
		else 
		processor.release();
	}
	catch(std::exception & e){
		ns_ex ex(e);
		
		sql.clear_query();
		
		processor().mark_subject_as_busy(false,sql);
		//we have found an error, handle it by registering it in the
		//host_event log, and annotate the current job (and any associated images)
		//with a reference to the error that occurred.
		ns_64_bit error_id(push_scheduler.report_job_as_problem(job,ex,sql));
		sql.send_query("COMMIT");
		
		
		processor().mark_subject_as_problem(error_id,sql);

		image_server.performance_statistics.cancel_outstanding_jobs();
		
		processor.release();
		if (ex.type() == ns_memory_allocation)
			throw; //memory allocation errors can cause big, long-term problems, thus we need to pass
					//them downwards to be handled.
	}
	return true;
}

void ns_processing_job_scheduler::clear_heap(){pipeline->pipeline.clear_heap();}

void ns_processing_job_scheduler::init_pipeline(){
	pipeline = new ns_image_processing_pipline_holder(ns_pipeline_chunk_size);
}
void ns_processing_job_scheduler::destruct_pipeline(){
	if (pipeline != 0){
		delete pipeline;
		pipeline = 0;
	}
}


