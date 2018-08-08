#include <sys/timeb.h>
#include "ns_worm_browser.h"
#include "ns_time_path_image_analyzer.h"
#ifndef _WIN32
#include <sys/time.h>
#endif
#ifdef _WIN32
#include "resource.h"
#include <Intrin.h>
#endif
#include "ns_high_precision_timer.h"
#include "ns_experiment_storyboard.h"
#include "ns_analyze_movement_over_time.h"
#include "ns_hand_annotation_loader.h"
#define IDLE_THROTTLE_FPS 40
#define SCALE_FONTS_WITH_WINDOW_SIZE 0

bool output_debug_messages = false;
bool output_debug_file_opened = false;
std::ofstream debug_output;

void refresh_main_window_internal(void *);
void refresh_worm_window_internal(void *);
bool debug_handlers = false;

#include <FL/Fl_Box.H>



void ns_worm_browser_output_debug(const unsigned long line_number,const std::string & source, const std::string & message){
	if (!output_debug_messages)
		return;
	if (!output_debug_file_opened){
		debug_output.open("verbose_debug_output.txt");
		output_debug_file_opened = true;
		if (debug_output.fail()){
			cerr << "Could not open verbose debug output file";
			return;
		}
	}
	cerr << source << "::" << line_number << ": " << message << "\n";
	debug_output << source << "::" << line_number << ": " << message << "\n";
	debug_output.flush();
}

ns_lock menu_bar_processing_lock("ns_menu_bar_lock");
ns_worm_learner worm_learner;

void ns_set_menu_bar_activity(bool a);
void refresh_main_window();


ns_64_bit GetTime(void){
#ifdef _WIN32 
	struct _timeb t;
	_ftime(&t);
	return (1000*(ns_64_bit)t.time)+(ns_64_bit)t.millitm;
#else
	struct timeval t;
	gettimeofday(&t,NULL);
	return (1000 * (ns_64_bit)t.tv_sec)+ (ns_64_bit)t.tv_usec/1000;
#endif
}

ns_lock dndlock;

bool show_worm_window = false;
bool hide_worm_window = false;

ns_thread_return_type ns_handle_drag_and_drop_asynch(void * request_text){
	ns_acquire_for_scope<string> v(static_cast<string *>(request_text));
	try{
	//	dndlock.wait_to_acquire(__FILE__,__LINE__);//)
		//	return;
		string cur;
		for (unsigned int i = 0; i < v().size(); i++){
			if (v()[i]==0 || v()[i]=='\n'){
				worm_learner.handle_file_request(cur);
				cur.resize(0);
			}
			if (v()[i]==0) break;
			cur+=v()[i];
		}
		if (cur.size() != 0){
				worm_learner.handle_file_request(cur);
				report_changes_made_to_screen();
		}
	//	dndlock.release();
	}
	catch(ns_ex & ex){
		cerr << "Error: " << ex.text() << "\n";
	}
	ns_set_menu_bar_activity(true);
	return 0;
}

void ns_handle_drag_and_drop(){
	ns_set_menu_bar_activity(false);
	ns_thread handle_file_thread(ns_handle_drag_and_drop_asynch,new string(Fl::event_text()));
	handle_file_thread.detach();
}


void idle_main_window_update_callback(void *);
void idle_worm_window_update_callback(void *);

//structure of class lifted from example at 
// http://seriss.com/people/erco/fltk/#OpenGlSimpleWidgets


void request_rate_limited_window_redraw_from_main_thread();

class ns_worm_terminal_main_window;
class ns_worm_terminal_worm_window;

ns_worm_terminal_main_window * current_window;
ns_worm_terminal_worm_window * worm_window;

// OPENGL WINDOW CLASS
class ns_worm_terminal_gl_window : public Fl_Gl_Window {
	bool mouse_is_down;
	ns_vector_2i mouse_click_location;
	bool have_focus;

    void fix_viewport(unsigned long x, unsigned long y, int width,int height) {
        glLoadIdentity();
     
    	glShadeModel (GL_FLAT);
		glMatrixMode (GL_PROJECTION);    /* prepare for and then */ 
	    glLoadIdentity ();               /* define the projection */
	    glFrustum (-1.0, 1.0, -1.0, 1.0, /* transformation */
	                  5, 20.0); 
	    glMatrixMode (GL_MODELVIEW);  /* back to modelview matrix */
	    glViewport (0,0, width,height);      /* define the viewport */

    }
    // DRAW METHOD
    void draw() {
        if (!valid()) { 
			valid(1); 
			fix_viewport(x(),y(),w(), h()); 
			glClearColor(1, 1, 1, 1); 
			 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffer
			 //glLoadIdentity ();             /* clear the matrix */
			 glTranslatef (0.0, 0.0, -5); /* viewing transformation */
			 glScalef (1.0, 1.0, 1.0);      /* modeling transformation */
			 //glColor3f(0,0,0);
		}      

		  try{
				worm_learner.update_main_window_display();	 
		  }
		  catch(std::exception & exception){
			ns_ex ex(exception);
				cerr << ex.text() << "\n";
				exit(1);
			}
    }   
	int handle(int state){
		if (Fl_Gl_Window::handle(state) != 0)
			return 1;
		if (debug_handlers) cout << "g\n";
		switch(state){
			case FL_FOCUS: {
				have_focus = true;
				int a = Fl_Gl_Window::handle(state); 
				//schedule_repeating_callback(0);
				report_changes_made_to_screen();
				return a; 
			}
			case FL_UNFOCUS: {
				have_focus = false;
				int a = Fl_Gl_Window::handle(state);
			//	report_changes_made_to_screen();
				return a;
			}
			//case FL_PUSH:
			//case FL_DRAG:
			//case FL_RELEASE:
			case FL_DND_ENTER:
			case FL_DND_RELEASE:
			case FL_DND_LEAVE:
			case FL_DND_DRAG:
				return 1;
			case FL_PASTE:
				ns_handle_drag_and_drop();
				report_changes_made_to_screen();
				return 1;
		}

		try{
			int button(Fl::event_button());
		//	cerr << "B:" << button;
			if(button == FL_LEFT_MOUSE || button == FL_RIGHT_MOUSE || mouse_is_down){
	
			
			ns_button_press press;
			press.right_button = false;
			if (button == FL_RIGHT_MOUSE)
				press.right_button = true;

			press.shift_key_held = Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R);
			press.control_key_held = Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R);
			press.screen_position = ns_vector_2i(Fl::event_x(),Fl::event_y());
			if (mouse_is_down)
				press.screen_distance_from_click_location = press.screen_position - mouse_click_location;
			else press.screen_distance_from_click_location = ns_vector_2i(0,0);

				switch(state){
			
					case FL_PUSH:{
						press.click_type = ns_button_press::ns_down;
						mouse_click_location = press.screen_position;
						mouse_is_down = true;
						worm_learner.touch_main_window_pixel(press);		

						report_changes_made_to_screen();
						return 1;
					}
					case FL_RELEASE:{
						press.click_type = ns_button_press::ns_up;
						mouse_is_down = false;
						worm_learner.touch_main_window_pixel(press);

						report_changes_made_to_screen();
						return 1;
					}
					case FL_DRAG:{
						press.click_type = ns_button_press::ns_drag;
						if (mouse_is_down && (abs(mouse_click_location.x-press.screen_position.x) > 4 || abs(mouse_click_location.y - press.screen_position.y) > 4))
								worm_learner.touch_main_window_pixel(press);

						report_changes_made_to_screen();
						return 1;
					} 

				}
			}
		}
		catch(std::exception & exception){
			ns_ex ex(exception);
			cerr << ex.text() << "\n";
			return 1;
		}
		//cerr << "Could not handle " << state << "\n";
		int a = Fl_Gl_Window::handle(state);
		//report_changes_made_to_screen();
		return a;
	}
	
public:
	
	// HANDLE WINDOW RESIZING
    void resize(int X,int Y,int W,int H) {
      //cerr << "GL"<<W << "," << H << "\n";
        Fl_Gl_Window::resize(X,Y,W,H);
		if (W != w() || H != h()){
	//		cerr << W << "x" << H << " from " << w() << "x" << h() << "\n";
       		fix_viewport(X,Y,W,H);
		}
   //     redraw();
    }

    // OPENGL WINDOW CONSTRUCTOR
    ns_worm_terminal_gl_window(int X,int Y,int W,int H,const char*L="worm window") :Fl_Gl_Window(X,Y,W,H,L),mouse_is_down(false),mouse_click_location(0,0),have_focus(false) {
        end();
    }
};

void ns_start_death_time_annotation(ns_worm_learner::ns_behavior_mode m, const ns_experiment_storyboard_spec::ns_storyboard_flavor & f){
	if (worm_learner.start_death_time_annotation(m,f))
		ns_set_main_window_annotation_controls_activity(true);
}


// OPENGL WINDOW CLASS
class ns_worm_gl_window : public Fl_Gl_Window {
	bool mouse_is_down;
	ns_vector_2i mouse_click_location;
	bool have_focus;

    void fix_viewport(int width,int height) {
        glLoadIdentity();
     
    	glShadeModel (GL_FLAT);
		glMatrixMode (GL_PROJECTION);    /* prepare for and then */ 
	    glLoadIdentity ();               /* define the projection */
	    glFrustum (-1.0, 1.0, -1.0, 1.0, /* transformation */
	                  5, 20.0); 
	    glMatrixMode (GL_MODELVIEW);  /* back to modelview matrix */
	    glViewport (0, 0, width,height);      /* define the viewport */

    }
    // DRAW METHOD
    void draw() {
        if (!valid()) { 
			valid(1); 
			fix_viewport(w(), h()); 
			glClearColor(1, 1, 1, 1); 
			 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffer
			 glLoadIdentity ();             /* clear the matrix */
			 glTranslatef (0.0, 0.0, -5.0); /* viewing transformation */
			 glScalef (1.0, 1.0, 1.0);      /* modeling transformation */
			 glColor3f(0,0,0);
		}      

		  try{
				worm_learner.update_worm_window_display();
		  }
		  catch(std::exception & exception){
			ns_ex ex(exception);
				cerr << ex.text() << "\n";
				exit(1);
			}
    }    
	int handle(int state){

		if (debug_handlers) cout << "l";
		switch(state){
		case FL_FOCUS: {
			have_focus = true;
			int a = Fl_Gl_Window::handle(state);
			return a;
		}
			case FL_UNFOCUS: {
				have_focus = false;
				int a = Fl_Gl_Window::handle(state);
				return a;
			}
		}

		try {
			int button(Fl::event_button());
			if (button == FL_LEFT_MOUSE || button == FL_RIGHT_MOUSE) {

				ns_button_press press;
				press.right_button = false;
				if (button == FL_RIGHT_MOUSE)
					press.right_button = true;

				press.shift_key_held = Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R);
				press.control_key_held = Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R);
				press.screen_position = ns_vector_2i(Fl::event_x(), Fl::event_y()) / worm_learner.worm_window.display_rescale_factor;

				switch (state) {

				case FL_PUSH: {
					press.click_type = ns_button_press::ns_down;
					mouse_click_location = press.screen_position;
					mouse_is_down = true;
					worm_learner.touch_worm_window_pixel(press);
					report_changes_made_to_screen();
					return 1;
				}
				case FL_RELEASE: {
					press.click_type = ns_button_press::ns_up;
					mouse_is_down = false;
					worm_learner.touch_worm_window_pixel(press);
					report_changes_made_to_screen();
					return 1;
				}
				case FL_DRAG: {
					press.click_type = ns_button_press::ns_drag;
					if (mouse_is_down && (abs(mouse_click_location.x - press.screen_position.x) > 4 || abs(mouse_click_location.y - press.screen_position.y) > 4))
						worm_learner.touch_worm_window_pixel(press);
					report_changes_made_to_screen();
					return 1;
				}

				}
			//	throw ns_ex("Unhandled gl window mouse event");
			}
		}
		catch(std::exception & exception){
			ns_ex ex(exception);
			cerr << ex.text() << "\n";
			return 1;
		}

		//cerr << "Could not handle " << state << "\n";
	/*	if (state == FL_SHOW) {
			show();
			return 1;
		}*/
	//		cerr << "WHA";
		int a = Fl_Gl_Window::handle(state);
		return a;
	}
	
public:
	// HANDLE WINDOW RESIZING
    void resize(int X,int Y,int W,int H) {
		Fl::lock();
		try {
			Fl_Gl_Window::resize(X, Y, W, H);
			if (W != w() || H != h()) {
				cerr << W << "x" << H << " from " << w() << "x" << h() << "\n";
				fix_viewport(W, H);
			}
		//	redraw();
			Fl::unlock();
		}
		catch (...) {
			Fl::unlock();
		}
    }

    // OPENGL WINDOW CONSTRUCTOR
    ns_worm_gl_window (int X,int Y,int W,int H,const char*L=0) : Fl_Gl_Window(X,Y,W,H,L),mouse_is_down(false),mouse_click_location(0,0),have_focus(false) {
       //xxxx
		end();
    }
};

struct ns_menu_item_options{
	ns_menu_item_options(const std::string & t, bool inactive_):text(t),inactive(inactive_){}
	ns_menu_item_options(const std::string & t):text(t),inactive(0){}
	ns_menu_item_options():inactive(0){}
	std::string text;
	bool inactive;
};
typedef void (*ns_menu_action)(const std::string & data);
struct ns_menu_item_spec{
  ns_menu_item_spec():flags(0),shortcut(0),action(0){}
	ns_menu_item_spec(const ns_menu_action a,const std::string & t,const int s=0,const int f=0):
		action(a),title(t),shortcut(s),flags(f){}

	ns_menu_action action;
	const std::string title;
	std::vector<ns_menu_item_options> options;
	int shortcut;
	int flags;
};

Fl_Menu_Bar * get_menu_bar();
class ns_worm_terminal_main_menu_organizer;
ns_worm_terminal_main_menu_organizer * get_menu_handler();

void all_menu_callback(Fl_Widget*w, void*data);

class ns_menu_organizer;

struct ns_asynch_menu_request{
	ns_asynch_menu_request(ns_menu_organizer * o, const std::string & r):organizer(o),request(r){}
	ns_menu_organizer * organizer;
	std::string request;

};

struct ns_menu_organizer_callback_data{
	Fl_Menu_Bar * bar;
	ns_menu_organizer * organizer;
};

class ns_menu_organizer{
protected:
	void add(const ns_menu_item_spec & item){
			menu_ordering.push_back(&(menu_actions.insert(std::pair<string,ns_menu_item_spec>(item.title,item)).first->second));
		}
	void clear(){
		menu_ordering.resize(0);
		menu_actions.clear();
	}
	
	typedef std::map<string,ns_menu_item_spec> ns_menu_actions;
	ns_menu_actions menu_actions;
	std::vector<ns_menu_item_spec *> menu_ordering;	

	static ns_thread_return_type handle_dispatch_request_asynch(void * request){

		ns_asynch_menu_request * r(static_cast<ns_asynch_menu_request *>(request));
		try{
			if(!r->organizer->asynch_lock.try_to_acquire(__FILE__,__LINE__)){
				cerr << "Too many simultaneous requests\n";
				delete r;
				return 0;
			}
			ns_set_menu_bar_activity(false);
			r->organizer->dispatch_request(r->request);
			r->organizer->asynch_lock.release();
			ns_set_menu_bar_activity(true);
			delete r;
			 
		}
		catch(ns_ex & ex){
			delete r;
			cerr << "Error in asynchronous thread: " << ex.text();
#ifdef _WIN32
			return 1;
#else
			// return type is void *: no simple/clean way to report errors
			return 0;
#endif
		}
		return 0;
	}

	ns_lock asynch_lock;
	ns_menu_organizer_callback_data callback_data;
public:
	ns_menu_organizer():asynch_lock("menu_organizer_asynch_lock"){}

	void build_menus(Fl_Menu_Bar & bar){
		callback_data.organizer = this;
		callback_data.bar = &bar;

		for (unsigned int i = 0; i < menu_ordering.size(); i++){
			if (menu_ordering[i]->options.size() == 0)
				bar.add(menu_ordering[i]->title.c_str(),menu_ordering[i]->shortcut,all_menu_callback,&callback_data,menu_ordering[i]->flags);
			for (unsigned int j = 0; j < menu_ordering[i]->options.size(); j++){
				bar.add((menu_ordering[i]->title + "/" + menu_ordering[i]->options[j].text).c_str(),0,all_menu_callback,&callback_data,menu_ordering[i]->options[j].inactive?FL_MENU_INACTIVE:0);
			}
		}
	}

	static std::string remove_menu_formatting(const string & s){
		string ret;
		for (unsigned int i = 0; i < s.size(); ++i){
			if (s[i]!='_' || i > 0 && (s[i-1] != '/')) ret+=s[i];
		}
		return ret;
	}
	void dispatch_request_asynch(const std::string & request){
	//		cerr << "To Many things at once.\n";
	//		return;
	//	}
		ns_asynch_menu_request * req(new ns_asynch_menu_request(this,request));
		ns_thread thread;
		thread.run(handle_dispatch_request_asynch,req);
	}

	void dispatch_request(const std::string & request) const{
		

		try{
			for (ns_menu_actions::const_iterator p = menu_actions.begin(); p != menu_actions.end(); ++p){
				if (remove_menu_formatting(p->second.title) == request){
					cerr << "Action: " << request << "\n";
					p->second.action("");
					return;
				}
			}
			//look to see if an item has been chosen from a menu with multiple options
			for (ns_menu_actions::const_iterator p = menu_actions.begin(); p != menu_actions.end(); ++p){
				const string suffix(remove_menu_formatting(p->second.title));
				string::size_type l(request.find(suffix));
				if (l != string::npos){
					string::size_type d(request.find_last_of("/"));
					cerr << "Action: " << request << "\n";
					p->second.action(request.substr(d+1,string::npos));
					return;
				}
			}
			throw ns_ex("Could not handle menu request:: ") << request;
		}
		catch(ns_ex & ex){
			cerr << "Error: " << ex.text();
			ns_alert_dialog d;
			d.text = ex.text();
			// d.act();
			ns_run_in_main_thread<ns_alert_dialog> dd(&d);
			
			/*MessageBox(
				0,
				ex.text().c_str(),
				"Worm Browser",
				MB_TASKMODAL | MB_ICONEXCLAMATION| MB_DEFBUTTON1 | MB_TOPMOST);
			}*/
		}
	} 

};

//never used?
//HANDLE ns_main_thread_id(0); 


std::string ns_get_input_string(const std::string title,const std::string default_value){
	ns_text_dialog td;
	td.grid_text.push_back("WHA");
	td.grid_text.push_back("BAH");
	td.title = "Warning";
	ns_run_in_main_thread_custom_wait<ns_text_dialog> n(&td);

	
	ns_choice_dialog c;
	c.title = "What do you want to do with this experiment schedule?";
	c.option_1 = "Generate a Summary File";
	c.option_2 = "Run it!";
	c.option_3 = "Cancel";
	ns_run_in_main_thread<ns_choice_dialog> b(&c);

	ns_file_chooser d;
	d.dialog_type = Fl_Native_File_Chooser::BROWSE_DIRECTORY;
	d.title = "Choose the desired experiment's backup directory";
	d.default_filename = "";
	ns_run_in_main_thread<ns_file_chooser> g(&d);
	
	ns_input_dialog is;
	is.title = "GAR";
	is.default_value = "ROR";
	ns_run_in_main_thread<ns_input_dialog> a(&is);
	return is.result;
}

void ns_quit();



class ns_worm_terminal_main_menu_organizer : public ns_menu_organizer{

	/*static void run_animation_trial(const std::string & value){
		ns_dir dir;
		string d("Y:\\image_server_storage\\partition_000\\2010_01_13_daf16_uno\\rise_c\\1\\movement_posture_vis");
		dir.load(d);
		sort(dir.files.begin(),dir.files.end());
		ns_image_standard im;
		for (unsigned int i = 0; i < dir.files.size(); i++){
			string filename(d + DIR_CHAR_STR + dir.files[i]);
			cerr << "Loading " << filename << "\n";
			worm_learner.load_file(filename);
			worm_learner.draw();
		}
	}*/

	/*****************************
	Worm Detection Tasks
	*****************************/
	static void calculate_erosion_gradient(const std::string & value){worm_learner.calculate_erosion_gradient();}
	static void show_objects(const std::string & value){worm_learner.detect_and_output_objects();}
	static void detect_worms(const std::string & value){worm_learner.process_contiguous_regions();}
	static void show_region_edges(const std::string & value){worm_learner.show_edges();}
	static void view_region_collage(const std::string & value){worm_learner.make_object_collage();}
	static void view_spine_collage(const std::string & value){worm_learner.make_spine_collage(worm_learner.output_svg_spines);}
	static void view_spine_collage_stats(const std::string & value){worm_learner.make_spine_collage_with_stats(worm_learner.output_svg_spines);}
	static void view_reject_spine_collage(const std::string & value){worm_learner.make_reject_spine_collage(worm_learner.output_svg_spines);}
	static void view_reject_spine_collage_stats(const std::string & value){worm_learner.make_reject_spine_collage_with_stats(worm_learner.output_svg_spines,worm_learner.worm_visualization_directory());}
	static void output_feature_distributions(const std::string & value){

		ns_image_file_chooser im_cc;
		im_cc.choose_directory();
		im_cc.title = "Which directory should distributions be written?";
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);


		worm_learner.output_distributions_of_detected_objects(im_cc.result);
	
	}
	static void calculate_slow_movement(const std::string & value){
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("TIF (*.tif)","tif"));
		foo.push_back(dialog_file_type("JPEG (*.jpg)","jpg"));
		foo.push_back(dialog_file_type("JPEG2000 (*.jp2)","jp2"));
		std::string filename = open_file_dialog("Load Image",foo);*/
		if (im_cc.chosen){
			worm_learner.characterize_movement(im_cc.result);
		}
	}
	/*****************************
	Machine Learning Tasks
	*****************************/

	static void start_death_aligned_posture_annotation(const std::string & value){
		ns_start_death_time_annotation(ns_worm_learner::ns_annotate_death_times_in_death_aligned_posture,ns_experiment_storyboard_spec::ns_number_of_flavors);
	}		
	static void start_time_aligned_posture_annotation(const std::string & value){
		ns_start_death_time_annotation(ns_worm_learner::ns_annotate_death_times_in_time_aligned_posture,ns_experiment_storyboard_spec::ns_number_of_flavors);
	}	
	static void start_storyboard_annotation(const string & flavor_str, const std::string & subject){

		if (subject == "Testing") {
			while (true) {
				worm_learner.worm_launch_finished = false;
				try {
					worm_learner.storyboard_annotater.load_random_worm();
				}
				catch (ns_ex & ex) {
					cerr << ex.text() << "\n";
				}
				while (!worm_learner.worm_launch_finished) ns_thread::sleep_milliseconds(100);
				ns_thread::sleep_milliseconds(1000);
			}
		}
		ns_experiment_storyboard_spec::ns_storyboard_flavor flavor;
		if (flavor_str.find("All") != flavor_str.npos)
			flavor = ns_experiment_storyboard_spec::ns_inspect_for_multiworm_clumps;
		else if (flavor_str.find("Immediately") != flavor_str.npos)
			flavor = ns_experiment_storyboard_spec::ns_inspect_for_non_worms;
		else throw ns_ex("Unkown Storyboard flavor");
		if (subject.find("Single Plate") != subject.npos)
			ns_start_death_time_annotation(ns_worm_learner::ns_annotate_storyboard_region,flavor);
		else if (subject.find("Experiment") != subject.npos)
			ns_start_death_time_annotation(ns_worm_learner::ns_annotate_storyboard_experiment,flavor);
		else throw ns_ex("Unknown Storyboard Subject type:") << subject;
	}
	static void start_storyboard_annotation_whole_experiment(const std::string & value){
		start_storyboard_annotation(value,"Experiment");
	}
	static void start_storyboard_annotation_testing(const std::string & value) {
		start_storyboard_annotation("", "Testing");
	}
	static void start_storyboard_annotation_plate(const std::string & value){
		start_storyboard_annotation(value,"Single Plate");
	}

	static void start_death_time_region_annotation(const std::string & value){
		cerr << "Nothing here yet.\n";
	}
	static void stop_posture_annotation(const std::string & value){
		worm_learner.stop_death_time_annotation();
		refresh_main_window();
	}

	static void compare_machine_and_by_hand_annotations(const std::string & value){
		worm_learner.compare_machine_and_by_hand_annotations();
	}
	static void generate_survival_curve_from_hand_annotations(const std::string & value){
		worm_learner.generate_survival_curve_from_hand_annotations();
	}
	static void generate_detailed_animal_data_file(const std::string & value){
		worm_learner.generate_detailed_animal_data_file();
	}

	static void output_learning_set(const std::string & value){
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen) worm_learner.output_learning_set(im_cc.result);		
	}
	static void auto_output_learning_set(const std::string & value){
		// Again, there must be a better place to put these than the root dir
		#ifdef _WIN32
		std::string path = "c:\\worm_detection\\training_set\\a";
		#else
		std::string path = "/worm_detection/training_set/a";		
		#endif
		worm_learner.output_learning_set(path,true);
		}
	static void rethreshold_image_set(const std::string & value){
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("(*.*)","*"));
		std::string filename = open_file_dialog("Load File",foo);
		if (filename != "") */
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.training_file_generator.re_threshold_training_set(im_cc.result,worm_learner.get_svm_model_specification());
	}

	static void analyze_worm_position(const std::string & value){
		worm_learner.analyze_time_path(worm_learner.data_selector.current_region().region_id);
	}
	static void generate_training_set(const std::string & value){worm_learner.generate_training_set_image();}
	static void process_training_set(const std::string & value){worm_learner.process_training_set_image();}
	static void generate_SVM_training_data(const std::string & value){
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("(*.*)","*"));*/
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		std::string * filename = new std::string(im_cc.result);
		if (im_cc.chosen)
			worm_learner.train_from_data(*filename);
		else delete filename;
	}
	static void split_results(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Choose Result File";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.training_file_generator.split_training_set_into_different_regions(im_cc.result);
	}
	static void fix_headers_for_svm_training_set_images(const std::string & value){
		
		std::string problem_directory,reference_directory;
		{
			ns_file_chooser im_cc;
			im_cc.title = "Choose Directory to Fix";
			ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
			problem_directory  = im_cc.result;
			if (!im_cc.chosen)
				return;
		}
		{
			ns_file_chooser im_cc;
			im_cc.title = "Choose Reference Directory";
			ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
			reference_directory  = im_cc.result;
			if (!im_cc.chosen)
				return;
		}
		string output_directory = problem_directory + DIR_CHAR_STR + "fixed_images";
		ns_dir::create_directory_recursive(output_directory);
		ns_training_file_generator gen;
		gen.repair_xml_metadata(problem_directory,reference_directory,output_directory);

	}
	static void analyze_svm_results(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Choose Result File";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		std::string * filename = new std::string(im_cc.result);
		ns_thread t;
		if (im_cc.chosen) worm_learner.training_file_generator.plot_errors_on_freq(*filename);
		else delete filename;
	}
//	static void run_temporal_inference(const std::string & value){worm_learner.run_temporal_inference();}
	static void remove_duplicates_from_training_set(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Load Directory";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		worm_learner.training_file_generator.mark_duplicates_in_training_set(im_cc.result);
	}
	static void generate_region_subset_time_series(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Load Directory";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		worm_learner.output_subregion_as_test_set(im_cc.result);
	}
	static void load_region_as_new_experiment(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Load Directory";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		worm_learner.input_subregion_as_new_experiment(im_cc.result);
	}
	static void create_decimated_subset(const std::string & value){
	ns_file_chooser im_cc;
		im_cc.title = "Load Directory";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		worm_learner.decimate_folder(im_cc.result);
	}
	static void translate_fscore(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Load Directory";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		worm_learner.translate_f_score_file(im_cc.result);
	}
	/*****************************
	Copy and Paste
	*****************************/
	static void clipboard_copy(const std::string & value){

		worm_learner.copy_to_clipboard();
	}
	static void clipboard_paste(const std::string & value){worm_learner.paste_from_clipboard();}
	/*****************************
	Mask Management
	*****************************/
	static void masks_generate_composite(const std::string & value){
		const bool subregion_label_mask(value.find("ubregion") != value.npos);
		ns_file_chooser im_cc;
		im_cc.save_file();
		im_cc.title = "Save Experiment Region Mask";
		if (subregion_label_mask)
			im_cc.title = "Save Subregion Label Mask";
		im_cc.filters.push_back(ns_file_chooser_file_type("TIF","tif"));
		im_cc.default_filename = worm_learner.data_selector.current_experiment_name();
		if (subregion_label_mask)
			im_cc.default_filename += "_subregion_label_mask.tif";
		else
			im_cc.default_filename +="_experiment_mask.tif";
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen) {
			if (subregion_label_mask)
				worm_learner.produce_mask_file(ns_bulk_experiment_mask_manager::ns_subregion_label_mask  ,im_cc.result);
			else 
				worm_learner.produce_mask_file(ns_bulk_experiment_mask_manager::ns_plate_region_mask, im_cc.result);

		}
	}
	static void masks_process_composite(const std::string & value){
		
		ns_file_chooser im_cc;
		im_cc.title = "Load Image Mask";
		im_cc.filters.push_back(ns_file_chooser_file_type("TIF","tif"));
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen) worm_learner.decode_mask_file(im_cc.result,im_cc.result+"_vis.tif");
	}
	static void masks_submit_composite(const std::string & value){
		const bool subregion_label_mask(value.find("ubregion") != value.npos); 
		if (subregion_label_mask)
			worm_learner.submit_mask_file_to_cluster(ns_bulk_experiment_mask_manager::ns_subregion_label_mask);
		else worm_learner.submit_mask_file_to_cluster(ns_bulk_experiment_mask_manager::ns_plate_region_mask);
	}

	static void open_individual_mask(const std::string & value){
		ns_file_chooser im_cc;
		im_cc.title = "Load Image Mask";
		im_cc.filters.push_back(ns_file_chooser_file_type("TIF","tif"));
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.load_mask(im_cc.result);
	}
	static void view_current_mask(const std::string & value){worm_learner.view_current_mask();}

	static void apply_mask_on_current(const std::string & value){
		worm_learner.apply_mask_on_current_image();
	}

	static void submit_individual_mask_to_server(const std::string & value){
		ns_image_server_captured_image im;
		try{
			int offset;
			im.from_filename(worm_learner.current_mask_filename,offset);
		}
		catch(...){
			cout << "Could not guess what sample this comes from.\n";
		}

		//string ip_address;
		//unsigned long port;
	//	worm_learner.get_ip_and_port_for_mask_upload(ip_address,port);
		ns_mask_info mask_info(worm_learner.send_mask_to_server(im.sample_id));
		ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));
		sql() << "UPDATE capture_samples SET mask_id=" << mask_info.mask_id << " WHERE id=" << im.sample_id;
		sql().send_query();
		sql() << "INSERT INTO processing_jobs SET image_id=" << mask_info.image_id << ", mask_id=" << mask_info.mask_id << ", "
				<< "op" << (unsigned int)ns_process_analyze_mask<< " = 1, time_submitted=" << ns_current_time() << ", urgent=1";
		sql().send_query();
		sql().send_query("COMMIT");

		ns_image_server_push_job_scheduler::request_job_queue_discovery(sql());
		sql.release();
	}
	/*****************************
	Samle Region Selection
	*****************************/
	static void save_current_areas(const std::string & value){
		worm_learner.save_current_area_selections();
	}
	static void clear_current_areas(const std::string & value){worm_learner.clear_areas();
																worm_learner.main_window.redraw_screen();}
	/*****************************
	Movement Detection Tasks
	*****************************/

	static std::string movement_type_menu_label(const ns_movement_data_source_type::type t){
		switch(t){
			case ns_movement_data_source_type::ns_time_path_image_analysis_data:	return "Time Path Image Analysis";
			case ns_movement_data_source_type::ns_time_path_analysis_data:			return "Time Path Spline Analysis";
			case ns_movement_data_source_type::ns_triplet_data:						return "Old-Style Triplet Analysis";
			case ns_movement_data_source_type::ns_triplet_interpolated_data:		return "Old-Style Triplet Interpolated Analysis";
		}
		return "";
	}
	static void generate_area_movement(const std::string & value){
		ns_movement_data_source_type::type t;
		//if (value == movement_type_menu_label(ns_movement_data_source_type::ns_time_path_image_analysis_data))
			t = ns_movement_data_source_type::ns_time_path_image_analysis_data;
		/*else if (value == movement_type_menu_label(ns_movement_data_source_type::ns_time_path_analysis_data))
			t = ns_movement_data_source_type::ns_time_path_analysis_data;
		else if (value == movement_type_menu_label(ns_movement_data_source_type::ns_triplet_data))
			t = ns_movement_data_source_type::ns_triplet_data;
		else if (value == movement_type_menu_label(ns_movement_data_source_type::ns_triplet_interpolated_data))
			t = ns_movement_data_source_type::ns_triplet_interpolated_data;
		else throw ns_ex("Unknown movement option: ") << value;*/
		
			worm_learner.compile_experiment_survival_and_movement_data(true,ns_movement_area_plot,t);
		
	}
	static void generate_survival_curves(const std::string & value){
		worm_learner.compile_experiment_survival_and_movement_data(true,ns_survival_curve,ns_movement_data_source_type::ns_all_data);
	}	
	static void generate_survival_curves_for_experiment_group(const std::string & value){
		ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));
		ns_experiment_region_selector_experiment_info info;
		worm_learner.data_selector.get_experiment_info(worm_learner.data_selector.current_experiment_id(),info);
		for (unsigned int i = 0; i < worm_learner.data_selector.experiment_groups.size(); i++){
			if (worm_learner.data_selector.experiment_groups.size() == 0)
				continue;
			else if (worm_learner.data_selector.experiment_groups[i][0].experiment_group_id != info.experiment_group_id)
				continue;
			for (unsigned int j = 0; j < worm_learner.data_selector.experiment_groups[i].size(); j++){
				try{
					worm_learner.data_selector.set_current_experiment(worm_learner.data_selector.experiment_groups[i][j].experiment_id,sql());
					cerr << "Loading information for experiment " << worm_learner.data_selector.current_experiment_name() << " (" << (j+1) << "/" << worm_learner.data_selector.experiment_groups[i].size() << ")\n";
					worm_learner.compile_experiment_survival_and_movement_data(true,ns_survival_curve,ns_movement_data_source_type::ns_all_data);
				}
				catch(ns_ex & ex){
					cerr << "Error: " << ex.text() << "\n";
				}
			}
			return;
		}
		throw ns_ex("Could not find experiment group in cache");
	}
	
	static void generate_location_plot(const std::string & value){worm_learner.compile_experiment_survival_and_movement_data(true,ns_movement_3d_path_plot,ns_movement_data_source_type::ns_time_path_analysis_data);}
	static void generate_posture_plot(const std::string & value){worm_learner.compile_experiment_survival_and_movement_data(true,ns_movement_3d_movement_plot,ns_movement_data_source_type::ns_time_path_analysis_data);}
	static void generate_timing_data(const std::string & value){
		worm_learner.output_device_timing_data(worm_learner.data_selector.current_experiment_id(),0);
	}
	static void simulate_multiple_worm_clusters(const std::string & value){
		if (value == slow_moving_name())
		worm_learner.simulate_multiple_worm_clumps(true,true);
		else
		worm_learner.simulate_multiple_worm_clumps(false,false);
	}
	static void generate_timing_data_all_exp(const std::string & value){
		ns_experiment_region_selector_experiment_info info;
		worm_learner.data_selector.get_experiment_info(worm_learner.data_selector.current_experiment_id(),info);
		worm_learner.output_device_timing_data(0,info.experiment_group_id);
	}
	static void generate_region_stats(const std::string & value){
		worm_learner.output_region_statistics(worm_learner.data_selector.current_experiment_id(),0);
	}
	static void export_experiment_data(const std::string & value){
		worm_learner.export_experiment_data(worm_learner.data_selector.current_experiment_id());
	}
	static void import_experiment_data(const std::string & value){
		ns_file_chooser d;
		d.dialog_type = Fl_Native_File_Chooser::BROWSE_DIRECTORY;
		d.title = "Locate the experiment's backup directory";
		d.default_filename = "";
		ns_run_in_main_thread<ns_file_chooser> g(&d);
		if (!d.chosen)
			return;
		ns_input_dialog is;
		is.title = "Enter in the new name of the new database to hold the experiment";
		is.default_value = "import_db";
		ns_run_in_main_thread<ns_input_dialog> a(&is);
		if (is.result.empty())
			return;
		if (!worm_learner.import_experiment_data(is.result,d.result,false)){
			string message("The database ");
			message += is.result;
			message += " already exists on the cluster.  Using it could currupt existing data, and probably won't work.  Are you sure you want to do this?";
			class ns_choice_dialog dialog;
			dialog.title = message;
			dialog.option_1 = "Overwrite Existing Data";
			dialog.option_2 = "Cancel";

			ns_run_in_main_thread<ns_choice_dialog> b(&dialog);
			//
			//int ret = MessageBox(
			////0,message.c_str(),"Database exists",
			//MB_TASKMODAL | MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_TOPMOST);
			//if (ret == IDCANCEL)
			//	return;
			//if (ret == IDYES){
			//	worm_learner.import_experiment_data(is.result,d.result,true);
			//}
			if (dialog.result == 1)
				worm_learner.import_experiment_data(is.result,d.result,true);
			
		}
	}
	static void generate_region_stats_for_all_regions_in_group(const std::string & value){
		ns_experiment_region_selector_experiment_info info;
		worm_learner.data_selector.get_experiment_info(worm_learner.data_selector.current_experiment_id(),info);
		worm_learner.output_region_statistics(0,info.experiment_group_id);
	}
	static void generate_morphology_stats(const std::string & value) {
		worm_learner.generate_morphology_statistics(worm_learner.data_selector.current_experiment_id());
	}


	static void generate_experiment_detailed_w_by_hand_movement_image_quantification_analysis_data(const std::string & value){worm_learner.generate_experiment_movement_image_quantification_analysis_data(ns_worm_learner::ns_quantification_detailed_with_by_hand);	}
	static void generate_experiment_abbreviated_movement_image_quantification_analysis_data(const std::string & value){worm_learner.generate_experiment_movement_image_quantification_analysis_data(ns_worm_learner::ns_quantification_abbreviated_detailed);	}
	
	static void generate_single_frame_posture_image_pixel_data(const std::string & value){
		worm_learner.generate_single_frame_posture_image_pixel_data((value.find("Plate") != std::string::npos));
	}
	static void generate_worm_markov_posture_model_from_by_hand_annotations(const std::string & value){worm_learner.generate_experiment_movement_image_quantification_analysis_data(ns_worm_learner::ns_build_worm_markov_posture_model_from_by_hand_annotations);	}
	
	static void generate_experiment_detailed_movement_image_quantification_analysis_data(const std::string & value){worm_learner.generate_experiment_movement_image_quantification_analysis_data(ns_worm_learner::ns_quantification_detailed);	}
	
	static void generate_experiment_summary_movement_image_quantification_analysis_data(const std::string & value){worm_learner.generate_experiment_movement_image_quantification_analysis_data(ns_worm_learner::ns_quantification_summary);	}
	
	static void test_time_path_analysis_parameters(const std::string & value){
		worm_learner.test_time_path_analysis_parameters(worm_learner.data_selector.current_region().region_id);
	}
	static void generate_movement_image_analysis_optimization_data(const std::string & value){
		bool posture_req = value.find("Posture") != value.npos;
		bool size_req = value.find("Size") != value.npos;
		worm_learner.output_movement_analysis_optimization_data(2, ns_worm_learner::ns_v2,posture_req,size_req);
	}

	static void generate_training_set_from_by_hand_annotations(const std::string & value){worm_learner.generate_training_set_from_by_hand_annotation();}
	
	
	/*****************************
	Configuration Tasks
	*****************************/
	static void show_extra_menus(const std::string & value);
	static void do_not_overwrite_schedules(const std::string & value){worm_learner.overwrite_submitted_specification(false);}
	static void overwrite_schedules(const std::string & value){worm_learner.overwrite_submitted_specification(true);}
	static void do_not_overwrite_existing_masks(const std::string & value){worm_learner.overwrite_existing_masks(false);}
	static void overwrite_existing_masks(const std::string & value){worm_learner.overwrite_existing_masks(true);}
	static void generate_mp4(const std::string & value){worm_learner.generate_mp4(true);}
	static void generate_wmv(const std::string & value){worm_learner.generate_mp4(false);}
	static void update_sql_schema(const std::string & value){worm_learner.upgrade_tables();}
	
	static void create_experiment_from_filenames(const std::string & value){
		ns_choice_dialog c;
		c.title = "Rebuilding experiment metadata from disk may corrupt any existing database contents for this experiment.\nYou should back up the database prior to attempting this.";
		c.option_1 = "Continue";
		c.option_2 = "Cancel";
		ns_run_in_main_thread<ns_choice_dialog> b(&c);
		if (c.result != 1)
			return;
		ns_file_chooser cc;
		cc.choose_directory();
		cc.title = "Choose the directory that holds experiment data";
		cc.default_directory = image_server.long_term_storage_directory;
		ns_run_in_main_thread<ns_file_chooser> e(&cc);
		if (!cc.chosen)
			return;
		if (cc.result.empty())
			return;
		ns_64_bit new_experiment_id = worm_learner.create_experiment_from_directory_structure(cc.result,true);
		worm_learner.rebuild_experiment_samples_from_disk(new_experiment_id);
		worm_learner.rebuild_experiment_regions_from_disk(new_experiment_id);
	}

	static void rebuild_db_sample_data_from_filenames(const std::string & value){
		ns_choice_dialog c;
		c.title = "Rebuilding experiment metadata from disk may corrupt any existing database contents for this experiment.\nYou should back up the database prior to attempting this.";
		c.option_1 = "Continue";
		c.option_2 = "Cancel";
		ns_run_in_main_thread<ns_choice_dialog> b(&c);
		if (c.result != 1)
			return;
		worm_learner.rebuild_experiment_samples_from_disk(worm_learner.data_selector.current_experiment_id());
	}
	static void rebuild_db_region_data_from_filenames(const std::string & value){
		ns_choice_dialog c;
		c.title = "Rebuilding experiment metadata from disk may corrupt any existing database contents for this experiment.\nYou should back up the database prior to attempting this.";
		c.option_1 = "Continue";
		c.option_2 = "Cancel";
		ns_run_in_main_thread<ns_choice_dialog> b(&c);
		if (c.result != 1)
			return;
		worm_learner.rebuild_experiment_regions_from_disk(worm_learner.data_selector.current_experiment_id());
	}
	/*****************************
	Image Processing Tasks
	*****************************/
	static void spatial_median(const std::string & value){worm_learner.apply_spatial_average();}
	static void difference_threshold(const std::string & value){worm_learner.difference_threshold();}
	static void adaptive_threshold(const std::string & value){worm_learner.apply_threshold();}
	static void movement_threshold(const std::string & value){
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen) worm_learner.calculate_movement_threshold(im_cc.result);
	}
	static void movement_threshold_vis(const std::string & value){
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)worm_learner.calculate_movement_threshold(im_cc.result,true);
	}
	static void two_stage_threshold(const std::string & value){worm_learner.two_stage_threshold();}
	static void two_stage_threshold_vis(const std::string & value){worm_learner.two_stage_threshold(true);}
	static void remove_large_objects(const std::string & value){worm_learner.remove_large_objects();}
	static void morph_manip(const std::string & value){worm_learner.run_binary_morpholgical_manipulations();}
	static void zhang_thinning(const std::string & value){worm_learner.zhang_thinning();}
	static void stretch_lossless(const std::string & value){worm_learner.stretch_levels();}
	static void stretch_lossy(const std::string & value){worm_learner.stretch_levels_approx();}
	static void compress_dark(const std::string & value){worm_learner.compress_dark_noise();}
	static void grayscale_from_blue(const std::string & value){worm_learner.grayscale_from_blue();}
	static void vertical_offset(const std::string & value){
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)worm_learner.calculate_vertical_offset(im_cc.result);
	}
	static void heat_map_overlay(const std::string & value){worm_learner.calculate_heatmap_overlay();}
	static void test_resample(const std::string & value){worm_learner.resize_image();}
	static void sharpen(const std::string & value){worm_learner.sharpen();}
	/*****************************
	Experiment and Model Selection
	*****************************/
	static void select_experiment(const std::string & value){
		ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));
		
		worm_learner.data_selector.set_current_experiment(value,sql());
		sql.release();
		update_region_choice_menu();
		update_strain_choice_menu();
		ns_update_main_information_bar("");
	}
	
	static void specifiy_model(const std::string & value){
		for (unsigned int i = 0; i < worm_learner.model_specifications.size(); ++i){
			if (worm_learner.model_specifications[i]().model_specification.model_name == value){
				worm_learner.set_svm_model_specification(worm_learner.model_specifications[i]);
				return;
			}
		}
		throw ns_ex("Could not recognize model name:") << value;
	}
	/*****************************
	File Tasks
	*****************************/
	
	static void set_database(const std::string & data){
		image_server.set_sql_database(data,false,&worm_learner.get_sql_connection());
		cerr << "Switching to database " << data << "\n";
		//ns_thread::sleep(15);
		get_menu_handler()->update_experiment_choice(*get_menu_bar());
	}
	static void file_open(const std::string & data){
		//cout << ns_get_input_string("TITLE","GOBER");
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("TIF (*.tif)","tif"));
		foo.push_back(dialog_file_type("JPEG (*.jpg)","jpg"));
		std::string filename = open_file_dialog("Load Image",foo);
		if (filename != ""){
			worm_learner.load_file(filename);
			worm_learner.draw();
		}*/
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen){
			worm_learner.load_file(im_cc.result);

			worm_learner.current_clipboard_filename = im_cc.result;
			worm_learner.draw();
		}
	}
	static void file_open_xml(const std::string & data){
		ns_file_chooser x;
		x.title = "Load XML Experiment Specification";
		x.filters.push_back(ns_file_chooser_file_type("XML","xml"));
		ns_run_in_main_thread<ns_file_chooser> run_mt(&x);
		if (x.chosen) worm_learner.handle_file_request(x.result);
	}
	static void file_save(const std::string & data){
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("TIF (*.tif)","tif"));
		foo.push_back(dialog_file_type("JPEG (*.jpg)","jpg"));
		std::string filename = save_file_dialog("Save Image",foo,"tif");		
		if (filename != "")
			worm_learner.save_current_image(filename);
			*/
		ns_image_file_chooser im_cc;
		im_cc.save_file();
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.save_current_image(im_cc.result);
	}
	static void show_worm(const std::string & data);
	static void file_open_16_bit_dark(const std::string & data){
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("TIF (*.tif)","tif"));
		foo.push_back(dialog_file_type("JPEG (*.jpg)","jpg"));
		std::string filename = open_file_dialog("Load 16-bit Image",foo);
		if (filename != "")*/
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.load_16_bit<ns_features_are_dark>(im_cc.result);
	}
	static void file_open_16_bit_light(const std::string & data){
		ns_image_file_chooser im_cc;
		ns_run_in_main_thread<ns_image_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.load_16_bit<ns_features_are_light>(im_cc.result);
	}
	static void file_quit(const std::string & data){
		ns_quit();
	}

	static void upload_strain_metadata(const std::string & data){
		/*std::vector<dialog_file_type> foo;
		foo.push_back(dialog_file_type("comma-separated value file (*.csv)","csv"));
		std::string filename = open_file_dialog("Load Strain Metadata File",foo);
		worm_learner.load_strain_metadata_into_database(filename);
		*/
		ns_file_chooser im_cc;
		im_cc.filters.push_back(ns_file_chooser_file_type("comma-separated value file (*.csv)","csv"));
		ns_run_in_main_thread<ns_file_chooser> run_mt(&im_cc);
		if (im_cc.chosen)
			worm_learner.load_strain_metadata_into_database(im_cc.result);
		
	}

public:

	/*****************************
	Menu Specification
	*****************************/
	void redraw_menus(Fl_Menu_Bar & bar) {
		bar.menu(NULL);
		clear();	
		add_menus();
		build_menus(bar);
		bar.redraw();
		::update_region_choice_menu();
	}
	void update_experiment_choice(Fl_Menu_Bar & bar){
		ns_sql & sql(worm_learner.get_sql_connection());
		ns_acquire_lock_for_scope lock(menu_bar_processing_lock,__FILE__,__LINE__);

		worm_learner.data_selector.load_experiment_names(sql);
		worm_learner.data_selector.set_current_experiment(-1,sql);
		redraw_menus(bar);
	}

	ns_worm_terminal_main_menu_organizer(){
		add_menus();
	}
	
	static std::string slow_moving_name(){return "Require Animals to be slow moving before entering clump";}

	void add_menus(){

		ns_menu_item_spec exp_spec(select_experiment,"&File/_Select Current Experiment");

		for (unsigned int i = 0; i < worm_learner.data_selector.experiment_groups.size(); i++){
			for (unsigned int j = 0; j < worm_learner.data_selector.experiment_groups[i].size(); j++){
				const bool last_in_group( j +1 ==  worm_learner.data_selector.experiment_groups[i].size());
				const bool last_group_in_list(i+1 == worm_learner.data_selector.experiment_groups.size());
				const std::string div((last_in_group&&!last_group_in_list)?"_":"");
				exp_spec.options.push_back(worm_learner.data_selector.experiment_groups[i][j].experiment_group_name + "/" + div + worm_learner.data_selector.experiment_groups[i][j].name);
			}
		}
		
		add(exp_spec);

		
	//	add(ns_menu_item_spec(run_animation_trial,"File/_Run Animation Trial"));
		add(ns_menu_item_spec(file_open,"File/Open Image",FL_CTRL+'o'));
		//add(ns_menu_item_spec(file_open_16_bit_dark,"File/Open 16-bit Image (dark)"));
		//add(ns_menu_item_spec(file_open_16_bit_light,"File/_Open 16-bit Image (light)"));
		add(ns_menu_item_spec(file_save,"File/_Save Image",FL_CTRL+'s'));
		//add(ns_menu_item_spec(show_worm, "File/Show worm window", FL_CTRL + 's'));
		add(ns_menu_item_spec(file_quit,"File/Quit",FL_CTRL+'q'));
		
		add(ns_menu_item_spec(file_open_xml,"&Image Acquisition/_Submit Experiment Schedule"));
		add(ns_menu_item_spec(save_current_areas,"Image Acquisition/Define Scan Areas/(Open Preview Capture Image and Draw Scan Areas)",0,FL_MENU_INACTIVE));
		add(ns_menu_item_spec(save_current_areas,"Image Acquisition/Define Scan Areas/Save Selected Scan Areas to Disk"));
		add(ns_menu_item_spec(clear_current_areas,"Image Acquisition/Define Scan Areas/Clear Selected Scan Areas"));

		//add(ns_menu_item_spec(clipboard_copy,"Clipboard/Copy",FL_CTRL+'c'));
		//add(ns_menu_item_spec(clipboard_paste,"Clipboard/Paste",FL_CTRL+'v'));

		add(ns_menu_item_spec(masks_generate_composite,"Image Acquisition/Define Sample Masks/Generate Experiment Mask Composite"));
		add(ns_menu_item_spec(masks_generate_composite,"Image Acquisition/Define Sample Masks/(Draw Plate Locations on Mask using Photoshop)",0,FL_MENU_INACTIVE));
		add(ns_menu_item_spec(masks_process_composite,"Image Acquisition/Define Sample Masks/Analyze Plate Locations Drawn on Experiment Mask Composite"));
		add(ns_menu_item_spec(masks_submit_composite,"Image Acquisition/Define Sample Masks/_Submit Analyzed Experiment Mask Composite to Cluster"));
		add(ns_menu_item_spec(open_individual_mask,"Image Acquisition/Define Sample Masks/Individual Sample Masks/Analyze Mask"));
	//	add(ns_menu_item_spec(view_current_mask,"Plate Locations/Define Sample Masks/Individual Sample Masks/View Current Mask"));
	//	add(ns_menu_item_spec(apply_mask_on_current,"Masks/Mask Analysis/Individual Masks/Apply Mask on Current Image"));
		add(ns_menu_item_spec(submit_individual_mask_to_server,"Image Acquisition/Define Sample Masks/Individual Sample Masks/Submit Analyzed Mask to Cluster"));
		
		add(ns_menu_item_spec(start_storyboard_annotation_whole_experiment,"&Validation/(Generate Storyboards Prior to Annotation)",0,FL_MENU_INACTIVE));
		ns_menu_item_spec st_an(start_storyboard_annotation_whole_experiment,"Validation/Browse Entire Experiment");
		st_an.options.push_back(ns_menu_item_options("Immediately After Each Worm's Death"));
		st_an.options.push_back(ns_menu_item_options("After All Worms Have Died"));
		add(st_an);
	//	add(ns_menu_item_spec(start_storyboard_annotation_whole_experiment, "&Validation/Test for storyboard memory bugs"));

		ns_menu_item_spec st_an2(start_storyboard_annotation_plate,"Validation/_Browse Single Plate");
		st_an2.options.push_back(ns_menu_item_options("Immediately After Each Worm's Death"));
		st_an2.options.push_back(ns_menu_item_options("After All Worms Have Died"));
		add(st_an2);
		add(ns_menu_item_spec(stop_posture_annotation,"Validation/Stop Annotation")); 
	//	add(ns_menu_item_spec(start_storyboard_annotation_testing, "Validation/Test for memory errors"));

		add(ns_menu_item_spec(generate_survival_curves,"&Data Files/_Death Times/Generate Death Times for Current Experiment"));
		add(ns_menu_item_spec(generate_survival_curves_for_experiment_group,"Data Files/Death Times/Generate Death Times for all Experiment in Experiment Group"));			 

		add(ns_menu_item_spec(generate_area_movement,"Data Files/Movement Data/_Generate Movement State Time Series"));
		//add(ns_menu_item_spec(generate_experiment_summary_movement_image_quantification_analysis_data,"Data/Movement/Generate Summary Time Path Image Analysis Quantification Data"));
		add(ns_menu_item_spec(generate_experiment_detailed_movement_image_quantification_analysis_data,"Data Files/Movement Data/Generate Posture Analysis Data/Machine Event Times"));
		add(ns_menu_item_spec(generate_experiment_detailed_w_by_hand_movement_image_quantification_analysis_data,"Data Files/Movement Data/Generate Posture Analysis Data/_By Hand Event Times"));
		add(ns_menu_item_spec(generate_experiment_abbreviated_movement_image_quantification_analysis_data,"Data Files/Movement Data/Generate Posture Analysis Data/Abbreviated"));
	
		ns_menu_item_spec st2(generate_single_frame_posture_image_pixel_data,"Data Files/Movement Data/Generate Single Frame Posture Image Data");
		st2.options.push_back(ns_menu_item_options("Experiment"));
		st2.options.push_back(ns_menu_item_options("Single Plate"));
		add(st2);
		add(ns_menu_item_spec(generate_timing_data,"Data Files/_Other Statistics/Generate Scanner Timing Data for Current Experiment"));
		add(ns_menu_item_spec(generate_timing_data_all_exp,"Data Files/Other Statistics/_Generate Scanner Timing Data for All Experiments in Group"));
		add(ns_menu_item_spec(generate_region_stats,"Data Files/Other Statistics/Generate Image Statistics for all regions in current Experiment"));
		add(ns_menu_item_spec(generate_region_stats_for_all_regions_in_group,"Data Files/Other Statistics/_Generate Image Statistics for all Regions in current Experiment Group"));
		add(ns_menu_item_spec(generate_morphology_stats,"Data Files/Other Statistics/Compile Worm Morphology Statistics for Current Experiment"));

		//add(ns_menu_item_spec(generate_survival_curve_from_hand_annotations,"&Calibration/Generate Survival Curves from by hand annotations"));
		add(ns_menu_item_spec(compare_machine_and_by_hand_annotations,"&Calibration/_Compare Storyboard annotations to fully-automated results"));
		add(ns_menu_item_spec(generate_movement_image_analysis_optimization_data,"Calibration/_Use Storyboard Annotations to Create improved Analysis Parameter Sets/Death Time Posture Changes"));
		add(ns_menu_item_spec(generate_movement_image_analysis_optimization_data, "Calibration/Use Storyboard Annotations to Create improved Analysis Parameter Sets/Death Time Size Changes"));
		add(ns_menu_item_spec(generate_movement_image_analysis_optimization_data, "Calibration/Use Storyboard Annotations to Create improved Analysis Parameter Sets/Both Posture and Size Changes"));
		add(ns_menu_item_spec(masks_generate_composite, "Calibration/_Define Subregions/Generate Subregion Mask Composite"));
		add(ns_menu_item_spec(masks_generate_composite, "Calibration/_Define Subregions/(Draw Plate Locations on Subregion Mask using Photoshop)", 0, FL_MENU_INACTIVE));
		add(ns_menu_item_spec(masks_process_composite, "Calibration/_Define Subregions/Analyze Subregion Labels Drawn on Subregion Mask Composite"));
		add(ns_menu_item_spec(masks_submit_composite, "Calibration/_Define Subregions/_Submit Analyzed Subregion Mask Composite to Cluster"));

		add(ns_menu_item_spec(test_time_path_analysis_parameters, "Calibration/Test various Position Analysis Models"));


		add(ns_menu_item_spec(export_experiment_data, "&Backup/Backup Database Contents for Current Experiment"));
		add(ns_menu_item_spec(import_experiment_data, "Backup/_Import Experiment from Backup"));
		add(ns_menu_item_spec(create_experiment_from_filenames, "Backup/Database Repair/_Create a new experiment in the database from images on disk"));
		add(ns_menu_item_spec(rebuild_db_sample_data_from_filenames, "Backup/Database Repair/Add missing sample images on disk into current experiment"));
		add(ns_menu_item_spec(rebuild_db_region_data_from_filenames, "Backup/Database Repair/Add missing region images on disk into current experiment"));
		//st4.options.push_back(ns_menu_item_options("Using Thermotolerance Parameter Range"));
		//st4.options.push_back(ns_menu_item_options("Using Lifespan Parameter Range"));
		//st4.options.push_back(ns_menu_item_options("Using Quiecent Lifespan Parameter Range"));
		//add(st4);
		
		//add(ns_menu_item_spec(generate_worm_markov_posture_model_from_by_hand_annotations,"Calibration/Build Hidden Markov Posture Model From By Hand Annotations"));

		if (worm_learner.show_testing_menus) {
			ns_menu_item_spec model_spec(specifiy_model, "&Testing/Worm Detection/Specify SVM Model");
			for (unsigned int i = 0; i < worm_learner.model_specifications.size(); i++)
				model_spec.options.push_back(ns_menu_item_options(worm_learner.model_specifications[i]().model_specification.model_name));

			add(model_spec);
			add(ns_menu_item_spec(spatial_median, "Testing/Image Processing/Spatial Median Filter"));
			//add(ns_menu_item_spec(difference_threshold,"Testing/Image Processing/Difference Threshold"));
			//add(ns_menu_item_spec(adaptive_threshold,"Testing/Image Processing/Adaptive Threshold"));
			//add(ns_menu_item_spec(movement_threshold,"Testing/Image Processing/Movement Threshold"));
			//add(ns_menu_item_spec(movement_threshold_vis,"Testing/Image Processing/Movement Threshold (vis)"));
			add(ns_menu_item_spec(two_stage_threshold, "Testing/Image Processing/Two Stage Threshold"));
			add(ns_menu_item_spec(two_stage_threshold_vis, "Testing/Image Processing/_Two Stage Threshold (vis)"));
			add(ns_menu_item_spec(remove_large_objects, "Testing/Image Processing/Remove Large Objects"));
			add(ns_menu_item_spec(morph_manip, "Testing/Image Processing/Morphological Manipulations"));
			add(ns_menu_item_spec(zhang_thinning, "Testing/Image Processing/_Zhang Thinning"));
			add(ns_menu_item_spec(stretch_lossless, "Testing/Image Processing/Stretch Dynamic Range (Lossless)"));
			add(ns_menu_item_spec(stretch_lossy, "Testing/Image Processing/_Stretch Dynamic Range (Lossy)"));
			add(ns_menu_item_spec(compress_dark, "Testing/Image Processing/Compress Dark Noise"));
			add(ns_menu_item_spec(grayscale_from_blue, "Testing/Image Processing/Grayscale from Blue Chanel"));
			add(ns_menu_item_spec(vertical_offset, "Testing/Image Processing/Calculate Vertical offset"));
			add(ns_menu_item_spec(heat_map_overlay, "Testing/Image Processing/_Calculate heat map overlay"));
			add(ns_menu_item_spec(test_resample, "Testing/Image Processing/Test Resample"));
			add(ns_menu_item_spec(sharpen, "Testing/Image Processing/Sharpen"));
			add(ns_menu_item_spec(calculate_erosion_gradient, "Testing/Image Processing/Calculate Erosion Gradient"));

			add(ns_menu_item_spec(show_objects, "Testing/Worm Detection/Show objects in image"));
			add(ns_menu_item_spec(detect_worms, "Testing/Worm Detection/Detect Worms"));
			add(ns_menu_item_spec(show_region_edges, "Testing/Worm Detection/Show Region Edges"));
			add(ns_menu_item_spec(view_region_collage, "Testing/Worm Detection/View Region Collage"));
			add(ns_menu_item_spec(view_spine_collage, "Testing/Worm Detection/View Spine Collage"));
			add(ns_menu_item_spec(view_spine_collage_stats, "Testing/Worm Detection/View Spine Collage with stats"));
			add(ns_menu_item_spec(view_reject_spine_collage, "Testing/Worm Detection/View Reject Spine Collage"));
			add(ns_menu_item_spec(view_reject_spine_collage_stats, "Testing/Worm Detection/View Reject Spine Collage with stats"));
			add(ns_menu_item_spec(output_feature_distributions, "Testing/Worm Detection/Output feature distributions"));
			add(ns_menu_item_spec(calculate_slow_movement, "Testing/Worm Detection/Calculate Slow Movement"));

			add(ns_menu_item_spec(output_learning_set, "Testing/Machine Learning/Output Learning Image Set"));
			add(ns_menu_item_spec(auto_output_learning_set, "Testing/Machine Learning/Auto Output Learning Image Set"));
			add(ns_menu_item_spec(rethreshold_image_set, "Testing/Machine Learning/_Rethreshold Learning Image Set "));
			add(ns_menu_item_spec(generate_training_set, "Testing/Machine Learning/Generate worm training set image"));
			add(ns_menu_item_spec(process_training_set, "Testing/Machine Learning/_Process worm training set image"));
			add(ns_menu_item_spec(generate_SVM_training_data, "Testing/Machine Learning/Generate SVM Training Data"));
			add(ns_menu_item_spec(split_results, "Testing/Machine Learning/Split results into multiple regions"));

			add(ns_menu_item_spec(fix_headers_for_svm_training_set_images, "Testing/Machine Learning/Fix Scrambled Training Set Metadata"));
			add(ns_menu_item_spec(analyze_svm_results, "Testing/Machine Learning/_Analyze SVM Results"));
			//	add(ns_menu_item_spec(run_temporal_inference,"Testing/Machine Learning/Run temporal inference"));
			add(ns_menu_item_spec(remove_duplicates_from_training_set, "Testing/Machine Learning/_Remove duplicates from training set"));
			add(ns_menu_item_spec(generate_region_subset_time_series, "Testing/Machine Learning/Generate Region Subset time series"));
			add(ns_menu_item_spec(load_region_as_new_experiment, "Testing/Machine Learning/_Load Region Subset time series as new experiment"));
			add(ns_menu_item_spec(create_decimated_subset, "Testing/Machine Learning/Create decimated subset of directory"));
			add(ns_menu_item_spec(translate_fscore, "Testing/Machine Learning/_translate f-score file"));
			add(ns_menu_item_spec(generate_training_set_from_by_hand_annotations, "Testing/Machine Learning/Generate Training Set from By Hand Movement Annotations"));

			add(ns_menu_item_spec(analyze_worm_position, "Testing/Movement Analysis/Analyze worm positions for current region"));
		}
	//	add(ns_menu_item_spec(compile_schedule_to_disk,"Config/Compile Submitted Capture Schedules to Disk"));
	//	add(ns_menu_item_spec(submit_schedule_to_db,"Config/_Submit Capture Schedules to Database"));	
		string version("Worm Browser v");
		version = version + ns_to_string(image_server.software_version_major()) + "." + ns_to_string(image_server.software_version_minor()) + "." + ns_to_string(image_server.software_version_compile()) + " (2017)";
		add(ns_menu_item_spec(set_database,string("&Config/") + version,0,FL_MENU_INACTIVE));
		add(ns_menu_item_spec(set_database,string("Config/_Nicholas Stroustrup, CRG"),0,FL_MENU_INACTIVE));
		//add(ns_menu_item_spec(set_database,string("Config/_ "),0,FL_MENU_INACTIVE));
		ns_menu_item_spec db_spec(set_database,"&Config/_Set Database");
		for (unsigned int i = 0; i < worm_learner.databases_available.size(); i++)
			db_spec.options.push_back(worm_learner.databases_available[i]);
		add(db_spec);
		add(ns_menu_item_spec(show_extra_menus, "Config/Set Behavior/_Show Image Analysis Diagnostic Tools"));
		add(ns_menu_item_spec(do_not_overwrite_existing_masks,"Config/Set Behavior/Do not overwrite existing sample masks"));
		add(ns_menu_item_spec(overwrite_existing_masks,"Config/Set Behavior/_Overwrite existing sample masks"));
		
		add(ns_menu_item_spec(generate_mp4,"Config/Set Behavior/Generate Mp4 Videos"));
		add(ns_menu_item_spec(generate_wmv,"Config/Set Behavior/_Generate WMV Videos"));
		add(ns_menu_item_spec(upload_strain_metadata,"Config/_Set Behavior/Upload Strain Metadata to the Database"));
		add(ns_menu_item_spec(update_sql_schema,"Config/Update database schema"));
}

	

};


class ns_worm_terminal_exclusion_menu_organizer : public ns_menu_organizer{
	static void pick_exclusion(const std::string & value){

		worm_learner.data_selector.set_censor_masking(ns_experiment_region_selector::censor_masking_from_string(value));
		::update_exclusion_choice_menu();
	}
public:
	std::string strain_menu_name(){return "File/_Select Current Strain";}
	void update_exclusion_choice(Fl_Menu_Bar & bar){
		bar.menu(NULL);
		clear();
		if (!worm_learner.data_selector.experiment_selected()){
			bar.deactivate();
			return;
		}
		if (worm_learner.data_selector.samples.size() == 0){
			bar.deactivate();
			return;
		}
		if (!worm_learner.data_selector.region_selected()){
			if (!worm_learner.data_selector.select_default_sample_and_region()){
				bar.deactivate();
				return;
			}
		}
		bar.activate();
		string title("");
		ns_experiment_region_selector::ns_censor_masking cur_masking(worm_learner.data_selector.censor_masking());
		ns_menu_item_spec spec(pick_exclusion,ns_experiment_region_selector::censor_masking_string(cur_masking));
		string sep;
		ns_experiment_region_selector::ns_censor_masking op[3] = {ns_experiment_region_selector::ns_show_all,
			ns_experiment_region_selector::ns_hide_censored,
			ns_experiment_region_selector::ns_hide_uncensored};

		for (unsigned int i = 0; i < 3; i++){
			if (op[i] == cur_masking)
				continue;
			else spec.options.push_back(ns_menu_item_options(ns_experiment_region_selector::censor_masking_string(op[i])));
		}
		add(spec);
		build_menus(bar);
		bar.redraw();
	}
};
class ns_worm_terminal_strain_menu_organizer : public ns_menu_organizer{
	static void pick_strain(const std::string & value){
		worm_learner.data_selector.select_strain(value);
		::update_strain_choice_menu();
		::update_region_choice_menu();
	}
public:
	std::string strain_menu_name(){return "File/_Select Current Strain";}
	void update_strain_choice(Fl_Menu_Bar & bar){
		bar.menu(NULL);
		clear();
		if (!worm_learner.data_selector.experiment_selected()){
			bar.deactivate();
			return;
		}
		if (worm_learner.data_selector.samples.size() == 0){
			bar.deactivate();
			return;
		}
		if (!worm_learner.data_selector.region_selected()){
			if (!worm_learner.data_selector.select_default_sample_and_region()){
				bar.deactivate();
				return;
			}
		}
		bar.activate();
		string title("");
		if (worm_learner.data_selector.strain_selected())
			title = worm_learner.data_selector.current_strain().device_regression_match_description();
		else title = "All Strains";
		ns_menu_item_spec spec(pick_strain,title);
		string sep;
		if (worm_learner.data_selector.strain_selected())
			spec.options.push_back(ns_menu_item_options("All Strains"));
		for (ns_experiment_region_selector::ns_experiment_strain_list::iterator p = worm_learner.data_selector.experiment_strains.begin(); p != worm_learner.data_selector.experiment_strains.end(); p++){
			if (p->first == title)
				continue;
			else spec.options.push_back(ns_menu_item_options(p->first));
		}
		add(spec);
		build_menus(bar);
		bar.redraw();
		/*bar.resize(experiment_bar_width(),
					h_-info_bar_height(),
					region_name_bar_width(),
					info_bar_height());*/
		/*if (worm_learner.current_behavior_mode() == ns_worm_learner::ns_annotate_death_times_in_time_aligned_posture)
				ns_start_death_time_annotation(ns_worm_learner::ns_annotate_death_times_in_time_aligned_posture);
		else if (worm_learner.current_behavior_mode() == ns_worm_learner::ns_annotate_death_times_in_death_aligned_posture)
				ns_start_death_time_annotation(ns_worm_learner::ns_annotate_death_times_in_death_aligned_posture);
		else if (worm_learner.current_behavior_mode() == ns_worm_learner::ns_annotate_death_times_in_region)
			/*do nothing 0;*/
	}
};


struct ns_asynch_region_picker{
	static ns_thread_return_type run_asynch(void * l){
		ns_asynch_region_picker launcher;
		std::string * region_name(static_cast<std::string *>(l));
		launcher.launch(*region_name);
		delete region_name;
		return 0;
	}
	void launch(const std::string & region_name){
		try{
			if (worm_learner.current_behavior_mode()==ns_worm_learner::ns_annotate_storyboard_region){
				if (!worm_learner.prompt_to_save_death_time_annotations())
					return;
				worm_learner.stop_death_time_annotation();
				worm_learner.data_selector.select_region(region_name);
				::update_region_choice_menu();
				worm_learner.start_death_time_annotation(ns_worm_learner::ns_annotate_storyboard_region,worm_learner.current_storyboard_flavor);
				return;
			}
			else{
				worm_learner.data_selector.select_region(region_name);
				::update_region_choice_menu();
			}
		}
		catch(ns_ex & ex){
			cerr << "Error loading worm info:" << ex.text();
			worm_learner.death_time_solo_annotater.close_worm();
			show_worm_window = false;
			ns_set_menu_bar_activity(true);
		}
	}
};


class ns_worm_terminal_region_menu_organizer : public ns_menu_organizer {
	static void pick_region(const std::string & value) {
		std::string *n(new std::string(value));
		ns_thread t(ns_asynch_region_picker::run_asynch, n);
		t.detach();
	}
public:
	std::string region_menu_name() { return "File/_Select Current Region"; }
	void update_region_choice(Fl_Menu_Bar & bar) {
		bar.menu(NULL);
		clear();
		if (!worm_learner.data_selector.experiment_selected()) {
			bar.deactivate();
			return;
		}
		if (worm_learner.data_selector.samples.size() == 0) {
			bar.deactivate();
			return;
		}
		if (!worm_learner.data_selector.region_selected())
			if (!worm_learner.data_selector.select_default_sample_and_region()) {
				bar.deactivate();
				return;
			}
		bar.activate();
		ns_menu_item_spec spec(pick_region, worm_learner.data_selector.current_region().display_name);
		string sep;

		//identify devices with no regions that match the strain selection, so we can gray them out.
		map<string, int> devices_with_valid_regions;
		if (worm_learner.data_selector.strain_selected()) {
			for (unsigned int i = 0; i < worm_learner.data_selector.samples.size(); i++) {
				for (unsigned int j = 0; j < worm_learner.data_selector.samples[i].regions.size(); j++) {
					if (worm_learner.data_selector.samples[i].regions[j].region_metadata == &worm_learner.data_selector.current_strain()) {
						devices_with_valid_regions[worm_learner.data_selector.samples[i].device] = 0;
						break;
					}
				}
			}
		}

		for (unsigned int i = 0; i < worm_learner.data_selector.samples.size(); i++) {

			if (worm_learner.data_selector.strain_selected()) {

				map<string, int>::const_iterator p = devices_with_valid_regions.find(worm_learner.data_selector.samples[i].device);

				if (p == devices_with_valid_regions.end()) {
					devices_with_valid_regions[worm_learner.data_selector.samples[i].device] = 1;
					spec.options.push_back(ns_menu_item_options(worm_learner.data_selector.samples[i].device, true));
					continue;
				}
				if (p->second == 1)
					continue;

				bool valid_regions_exist(false);
				for (unsigned int j = 0; j < worm_learner.data_selector.samples[i].regions.size(); j++) {
					if (worm_learner.data_selector.samples[i].regions[j].region_metadata == &worm_learner.data_selector.current_strain()) {
						valid_regions_exist = true;
						break;
					}
				}
				if (!valid_regions_exist) {
					spec.options.push_back(ns_menu_item_options(worm_learner.data_selector.samples[i].device + "/" + worm_learner.data_selector.samples[i].sample_name, true));
					continue;
				}
			}

			for (unsigned int j = 0; j < worm_learner.data_selector.samples[i].regions.size(); j++) {

				if (worm_learner.data_selector.samples[i].regions[j].region_id == worm_learner.data_selector.current_region().region_id)
					continue;
				const bool unselected_strain(worm_learner.data_selector.strain_selected() && worm_learner.data_selector.samples[i].regions[j].region_metadata != &worm_learner.data_selector.current_strain());

				spec.options.push_back(ns_menu_item_options(worm_learner.data_selector.samples[i].device + "/" + worm_learner.data_selector.samples[i].sample_name + "/" + worm_learner.data_selector.samples[i].regions[j].display_name, unselected_strain));

			}
		}
		add(spec);
		build_menus(bar);
		bar.redraw();
		/*bar.resize(experiment_bar_width(),
					h_-info_bar_height(),
					region_name_bar_width(),
					info_bar_height());*/
		if (worm_learner.current_behavior_mode() == ns_worm_learner::ns_annotate_death_times_in_time_aligned_posture)
			ns_start_death_time_annotation(ns_worm_learner::ns_annotate_death_times_in_time_aligned_posture, worm_learner.current_storyboard_flavor);
		else if (worm_learner.current_behavior_mode() == ns_worm_learner::ns_annotate_death_times_in_death_aligned_posture)
			ns_start_death_time_annotation(ns_worm_learner::ns_annotate_death_times_in_death_aligned_posture, worm_learner.current_storyboard_flavor);
		else if (worm_learner.current_behavior_mode() == ns_worm_learner::ns_annotate_death_times_in_region)
			/*do nothing*/0;
	}
};
void ns_handle_death_time_annotation_button(Fl_Widget * w, void * data);
void ns_handle_death_time_solo_annotation_button(Fl_Widget * w, void * data);

class ns_death_event_annotation_group : public Fl_Pack {
public:
	Fl_Button * forward_button,
			  * back_button,
			  * play_forward_button,
			  * play_reverse_button,
			  * stop_button,
			  * save_button;
	enum{button_width=24,button_height=24,window_width=168,window_height=24};
	int handle(int e){
		//don't claim keystrokes--we want them to be passed on to the glwindow for navigation
		switch(e){
			case FL_KEYDOWN:
				return 0;
			case FL_KEYUP:
				return 0;
		}
		return Fl_Pack::handle(e);
	}
	ns_death_event_annotation_group(int x,int y, int w, int h, bool add_save_button) : Fl_Pack(x,y,w,h) {
		type(Fl_Pack::HORIZONTAL);
		spacing(0);
		back_button 		= new Fl_Button(0*button_width,0,button_width,button_height,"@-2<");
		back_button->callback(ns_handle_death_time_annotation_button,
			new ns_image_series_annotater::ns_image_series_annotater_action(ns_death_time_posture_annotater::ns_back));
		play_reverse_button = new Fl_Button(1*button_width,0,button_width,button_height,"@-2<<");
		play_reverse_button->callback(ns_handle_death_time_annotation_button,
			new ns_image_series_annotater::ns_image_series_annotater_action(ns_death_time_posture_annotater::ns_fast_back));
		stop_button 		= new Fl_Button(2*button_width,0,button_width,button_height,"@-9square");
		stop_button->callback(ns_handle_death_time_annotation_button,
			new ns_image_series_annotater::ns_image_series_annotater_action(ns_death_time_posture_annotater::ns_stop));
		play_forward_button = new Fl_Button(3*button_width,0,button_width,button_height,"@-2>>");
		play_forward_button->callback(ns_handle_death_time_annotation_button,
			new ns_image_series_annotater::ns_image_series_annotater_action(ns_death_time_posture_annotater::ns_fast_forward));
		forward_button 		= new Fl_Button(4*button_width,0,button_width,button_height,"@-2>");
		forward_button->callback(ns_handle_death_time_annotation_button,
			new ns_image_series_annotater::ns_image_series_annotater_action(ns_death_time_posture_annotater::ns_forward));
		if (add_save_button){
			save_button 		= new Fl_Button(5*button_width,0,2*button_width,button_height,"Save");
			save_button->callback(ns_handle_death_time_annotation_button,
				new ns_image_series_annotater::ns_image_series_annotater_action(ns_death_time_posture_annotater::ns_save));
		}
		else{
			save_button=0;
		}
		end();
	//	clear_visible_focus();

	}
};
					

class ns_death_event_solo_annotation_group : public Fl_Pack {
public:
	Fl_Button * forward_button,
		*back_button,
		*play_forward_button,
		*play_reverse_button,
		*stop_button,
		*goto_death_time_button,
		*save_button,
		*visualization_button,
		*graph_button,
		*zoom_in_button,
		*zoom_out_button;
	Fl_Output * info_bar;
	enum{button_width=18,button_height=18,export_button_width=50,
		 visualization_button_width=32, graph_button_width = 50,space_width=9};
	static unsigned long all_buttons_width() {
		return 5 * button_width + visualization_button_width + graph_button_width + export_button_width+ space_width;
	}
	int handle(int e){
		//don't claim keystrokes--we want them to be passed on to the glwindow for navigation
		switch(e){
			case FL_KEYDOWN:
				return 0;
			case FL_KEYUP:
				return 0;
		}
		return Fl_Pack::handle(e);
	}

	void resize(int x, int y, int w, int h) {

		const int info_bar_x(all_buttons_width());

		int bw = button_width,
			vis_bw = visualization_button_width,
			graph_bw = graph_button_width,
			exp_bw = export_button_width;
		const bool squished = (w < info_bar_x);
		if (squished){
			bw = vis_bw= graph_bw= exp_bw = w / 8;
		}
		back_button->resize(x, y, bw, h);
		stop_button->resize(bw+x, y, bw, h);
		forward_button->resize(3* bw+x, y, bw, h);
		visualization_button->resize(3* bw + x, y, vis_bw, h);
		graph_button->resize(3 * bw + vis_bw+ x, y, graph_bw, h);
		save_button->resize(3 * bw + vis_bw+graph_bw + x, y, exp_bw, h);
		zoom_out_button->resize(3 * bw + vis_bw + graph_bw+exp_bw + x, y, bw, h);
		zoom_in_button->resize(4 * bw + vis_bw + graph_bw + exp_bw  + x, y, bw, h);
		if (squished) {
			info_bar->hide();
		}
		else {
			info_bar->resize(info_bar_x + x, y, w - info_bar_x, h);
			info_bar->show();
		}
		Fl_Pack::resize(x, y, w, h);
	}
	ns_death_event_solo_annotation_group(int x,int y, int w, int h) : Fl_Pack(x,y,w,h) {
		type(Fl_Pack::HORIZONTAL);
	//	spacing(0);
		//int button_width = 3;
	//	play_reverse_button = new Fl_Button(0*button_width,0,button_width,button_height,"@-2<<");
	//	play_reverse_button->callback(ns_handle_death_time_solo_annotation_button,
	//		new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_fast_back));
		back_button = new Fl_Button(1 * button_width, 0, button_width, button_height, "@-2<");
		back_button->callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_back));

		stop_button 		= new Fl_Button(2*button_width,0,button_width,button_height,"@-9square");
		stop_button->callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_stop));
		
		forward_button 		= new Fl_Button(3*button_width,0,button_width,button_height,"@-2>");
		forward_button->callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_forward));

		//play_forward_button = new Fl_Button(4 * button_width , 0, button_width, button_height, "@-2>>");
	//	play_forward_button->callback(ns_handle_death_time_solo_annotation_button,
		//	new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_fast_forward));
		visualization_button = new Fl_Button(4 * button_width+ space_width, 0, visualization_button_width, button_height, "Vis");
		visualization_button->callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_step_visualization));
		graph_button = new Fl_Button(4 * button_width+ visualization_button_width + space_width, 0, graph_button_width, button_height, "Graph");
		graph_button->callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_step_graph));

		save_button = new Fl_Button(4*button_width+ visualization_button_width+graph_button_width + space_width,0, export_button_width,button_height,"Export");
		save_button->  callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_write_quantification_to_disk));

		zoom_out_button = new Fl_Button(4 * button_width + visualization_button_width + graph_button_width + space_width+ export_button_width, 0, button_width, button_height, "-");
		zoom_out_button->callback(ns_handle_death_time_solo_annotation_button,

			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_time_zoom_out_step));
		zoom_in_button = new Fl_Button(5 * button_width + visualization_button_width + graph_button_width + space_width+ export_button_width, 0, button_width, button_height, "+");
		zoom_in_button->callback(ns_handle_death_time_solo_annotation_button,
			new ns_death_time_solo_posture_annotater::ns_image_series_annotater_action(ns_death_time_solo_posture_annotater::ns_time_zoom_in_step));

		const int info_bar_x(all_buttons_width());
		info_bar = new Fl_Output(info_bar_x,0,w- info_bar_x, button_height);
		info_bar->value("");
	//	info_bar->textsize(experiment_name_bar->textsize());
		info_bar->textfont(FL_HELVETICA);
		info_bar->deactivate();

		end();

	//	clear_visible_focus();

	}
};
			

void demand_window_redraw_from_main_thread();
// APP WINDOW CLASS
class ns_dialog_window : public Fl_Window {
public:
	Fl_Button* a,*b,*c;
    ns_dialog_window (int W,int H,const char*L=0) : Fl_Window(W,H,L){
		
		a = new Fl_Button(W/5,H/4,W/5,H/4,"Hello!");
		b = new Fl_Button(2*W/5,H/4,W/5,H/4,"There!");
		c = new Fl_Button(3*W/5,H/4,W/5,H/4,"Beautiful!");
  
	    end();
    }
	/*int handle(int state) {
		redraw();
		return Fl_Window::handle(state);
	};*/
};

/*
ns_thread_return_type asynch_redisplay(void *);*/

// APP WINDOW CLASS
class ns_worm_terminal_main_window : public Fl_Window {
	bool have_focus;
public:
    ns_worm_terminal_gl_window * gl_window;
	ns_death_event_annotation_group * annotation_group;
	Fl_Menu_Bar *main_menu;         
	Fl_Menu_Bar *region_menu;       
	Fl_Menu_Bar *exclusion_menu;  
	Fl_Menu_Bar *strain_menu;  
	Fl_Output * experiment_name_bar;
	//Fl_Output * region_name_bar;
	Fl_Output * info_bar;

	//ns_dialog_window * dialog_window;
//	Fl_Output * spacer_bar;
	ns_worm_terminal_main_menu_organizer * main_menu_handler;
	ns_worm_terminal_region_menu_organizer * region_menu_handler;
	ns_worm_terminal_strain_menu_organizer * strain_menu_handler;
	ns_worm_terminal_exclusion_menu_organizer * exclusion_menu_handler;

	static unsigned long menu_height() {return 23;}
	static unsigned long info_bar_height(){return 24;}

	static unsigned long experiment_bar_width(){return 200;}
	static unsigned long region_name_bar_width(){return 125;}
	static unsigned long exclusion_bar_width(){return 30;}
	static unsigned long strain_bar_width(){return 140;}

	static unsigned long border_height() {return 0;}
	static unsigned long border_width() {return 0;}
	~ns_worm_terminal_main_window(){
		ns_safe_delete(gl_window);
		ns_safe_delete(main_menu);
		ns_safe_delete(region_menu);
		ns_safe_delete(main_menu_handler);
		ns_safe_delete(region_menu_handler);
		ns_safe_delete(strain_menu_handler);
		ns_safe_delete(exclusion_menu_handler);
		ns_safe_delete(experiment_name_bar);
		ns_safe_delete(info_bar);
		ns_safe_delete(exclusion_menu);
	//	ns_safe_delete(drag_and_drop_box);
	}

	void update_information_bar(const std::string & status="-1"){
		experiment_name_bar->value(worm_learner.data_selector.current_experiment_name().c_str());
		cerr << "Selecting experiment " << worm_learner.data_selector.current_experiment_name().c_str() << "\n";
	/*	if (worm_learner.data_selector.region_selected() && worm_learner.data_selector.sample_selected()){
			region_name_bar->value((worm_learner.data_selector.current_sample().sample_name + "::" + 
								   worm_learner.data_selector.current_region().region_name).c_str());
		}*/
		if (status != "-1")
			info_bar->value(status.c_str());
	}

    ns_worm_terminal_main_window(int W,int H,const char*L=0) : Fl_Window(50,50,W,H,L), draw_animation(false),last_draw_animation(false),have_focus(false){
        // OpenGL window
        gl_window = new ns_worm_terminal_gl_window(0, menu_height(), w()-border_width(), h()-menu_height()-info_bar_height());
	
    	main_menu = new Fl_Menu_Bar(0,0,W,menu_height());
		
	
		float d(worm_learner.main_window.display_rescale_factor);
		float menu_d(worm_learner.main_window.display_rescale_factor);
		//do not scale fonts
		if (!SCALE_FONTS_WITH_WINDOW_SIZE)
			menu_d = 1;

		main_menu->textsize((main_menu->textsize()-2)*menu_d);
		main_menu->textfont(FL_HELVETICA);
		main_menu_handler = new ns_worm_terminal_main_menu_organizer();
		main_menu_handler->build_menus(*main_menu);		
	

		experiment_name_bar = new Fl_Output(0,
											h()*d-info_bar_height()*menu_d,
											(experiment_bar_width())*d,
											info_bar_height()*menu_d);
		experiment_name_bar->textsize((experiment_name_bar->textsize()-4)*menu_d);
		experiment_name_bar->textfont(FL_HELVETICA);

		region_menu = new Fl_Menu_Bar(experiment_bar_width()*d, H - info_bar_height()*menu_d, region_name_bar_width()*menu_d, info_bar_height()*menu_d);
		region_menu->textsize((region_menu->textsize() - 4)*menu_d);
		region_menu->textfont(FL_HELVETICA);

		region_menu_handler = new ns_worm_terminal_region_menu_organizer();
		region_menu_handler->update_region_choice(*region_menu);

		strain_menu = new Fl_Menu_Bar((int)(d*experiment_bar_width() + menu_d*region_name_bar_width()), (int)(H - info_bar_height()*menu_d), (int)(menu_d*strain_bar_width()), (int)(menu_d*info_bar_height()));
		exclusion_menu = new Fl_Menu_Bar((int)(d*experiment_bar_width() + menu_d*(region_name_bar_width() + strain_bar_width())), (int)(H - menu_d*info_bar_height()),(int)(menu_d*exclusion_bar_width()), (int)(menu_d*info_bar_height()));
		strain_menu_handler = new ns_worm_terminal_strain_menu_organizer();
		strain_menu_handler->update_strain_choice(*strain_menu);
		exclusion_menu_handler = new ns_worm_terminal_exclusion_menu_organizer();
		exclusion_menu_handler->update_exclusion_choice(*exclusion_menu);
		strain_menu->textsize(region_menu->textsize()*menu_d);
		strain_menu->textfont(FL_HELVETICA);
		exclusion_menu->textsize(region_menu->textsize()*menu_d);
		exclusion_menu->textfont(FL_HELVETICA);


		int left_buttons_width(d*experiment_bar_width() + menu_d*(region_name_bar_width() + strain_bar_width() + exclusion_bar_width()));
		//cerr << W - left_buttons_width - ns_death_event_annotation_group::window_width << " ";
		info_bar = new Fl_Output(left_buttons_width,
										    (int)(h() - menu_d*info_bar_height()),
											W- left_buttons_width - ns_death_event_annotation_group::window_width,
											(int)(info_bar_height()*menu_d));
		
		info_bar->textsize(experiment_name_bar->textsize());
		info_bar->textfont(FL_HELVETICA);
	

		annotation_group = new ns_death_event_annotation_group((W-ns_death_event_annotation_group::window_width)*d,
															  H*d - ns_death_event_annotation_group::window_height*menu_d,
															  (ns_death_event_annotation_group::window_width)*d,
															  (ns_death_event_annotation_group::window_height)*menu_d,true);

		annotation_group->deactivate();
		experiment_name_bar->box(FL_EMBOSSED_BOX);
		info_bar->box(FL_EMBOSSED_BOX);

		info_bar->deactivate();
		experiment_name_bar->deactivate();

		update_information_bar("Welcome!");

	

		experiment_name_bar->value(worm_learner.data_selector.current_experiment_name().c_str());

		info_bar->value("Welcome!");

	//	drag_and_drop_box = new ns_dnd_handling_widget(0,0,W, H);
	    end();
		
    }

	static ns_vector_2i image_window_size_difference(float menu_d){
	  if (!SCALE_FONTS_WITH_WINDOW_SIZE)
		  menu_d = 1;
		return ns_vector_2i(0,menu_d*(int)menu_height()+ menu_d*(int)info_bar_height());
	}

  void get_window_size_needed(int & w, int & h, float & d, float & menu_d){
	 d = worm_learner.main_window.display_rescale_factor;
		menu_d = worm_learner.main_window.display_rescale_factor;
		if (!SCALE_FONTS_WITH_WINDOW_SIZE)
			menu_d = 1;
	     
	//	worm_learner.main_window.image_size = ns_vector_2d(w_,h_);
	//	worm_learner.main_window.specified_gl_image_size = (worm_learner.main_window.image_size+
	//			image_window_size_difference());

		w = worm_learner.main_window.gl_image_size.x*d,
			h = worm_learner.main_window.gl_image_size.y*d+image_window_size_difference(worm_learner.main_window.display_rescale_factor).y ;
		if (w == 0)
			w = 600;
		if (h == 0)
			h = 800;
	    
  }
	void resize(int x, int y, int width, int height){
	  //cerr << "W" << width << "," << height << " : ";
		
		ns_acquire_lock_for_scope lock(worm_learner.main_window.display_lock,__FILE__,__LINE__);
		ns_vector_2i window_size;
		float d,menu_d;
		get_window_size_needed(window_size.x,window_size.y,d,menu_d);

		int w_ = worm_learner.main_window.gl_image_size.x,
			h_ = worm_learner.main_window.gl_image_size.y;

		//cerr << "ww" << window_size.x << ","<< window_size.y << "\n";
		Fl_Window::resize(x,y,window_size.x,window_size.y);
	//	drag_and_drop_box->resize(0, 0, window_size.x, window_size.y);
		gl_window->resize(0,menu_height()*menu_d,
			w_*d,
			h_*d);
	
		main_menu->resize(0,0,w_*d,menu_height()*menu_d);

		int bottom_bar_height(h_*d+menu_height()*menu_d);
		//cerr << "eb" << experiment_bar_width()*d << ","  << bottom_bar_height << "-" <<  bottom_bar_height+info_bar_height() << "\n";
		experiment_name_bar->resize(		0,
											bottom_bar_height,
											experiment_bar_width()*d,
											info_bar_height());
		region_menu->resize(experiment_bar_width()*d,
											bottom_bar_height,
											region_name_bar_width()*d,
										
											info_bar_height()*menu_d);
		strain_menu->resize((experiment_bar_width()+region_name_bar_width())*d,
											bottom_bar_height,
											strain_bar_width()*d,
											info_bar_height()*menu_d);
		exclusion_menu->resize((experiment_bar_width()+region_name_bar_width()+strain_bar_width())*d,
											bottom_bar_height,
											exclusion_bar_width()*d,
											info_bar_height()*menu_d);
		int left_buttons_width((experiment_bar_width() + region_name_bar_width() + strain_bar_width() + exclusion_bar_width())*d);
		info_bar->resize(left_buttons_width,
											bottom_bar_height,
											w_*d- left_buttons_width -(ns_death_event_annotation_group::window_width)*d,
											info_bar_height()*menu_d);

		annotation_group->resize((w_-ns_death_event_annotation_group::window_width)*d,
								bottom_bar_height,
								 ns_death_event_annotation_group::window_width*d,
								 ns_death_event_annotation_group::window_height*menu_d);

		lock.release();
		if (w() != w_ || h() != h_)
		  request_rate_limited_window_redraw_from_main_thread();

	}
	 
	void update_region_choice_menu(){
		region_menu_handler->update_region_choice(*region_menu);
		update_information_bar();
	
	}
	void update_exclusion_choice_menu(){
		exclusion_menu_handler->update_exclusion_choice(*exclusion_menu);
		update_information_bar();
	}
	void update_strain_choice_menu(){
		strain_menu_handler->update_strain_choice(*strain_menu);
		update_information_bar();
	}
	bool draw_animation;
	bool last_draw_animation;

	private:
		
		
	int handle(int state) {

		//cout << "MW" << state << "\n";
		if (debug_handlers) cout << "m";

		switch(state){ 
			case FL_FOCUS:
				have_focus = true;
				break;
			case FL_UNFOCUS:
				have_focus = false;
				break;
			case FL_KEYDOWN:{
				//if (have_focus){
					Fl::lock();	
					int c(Fl::event_key());
					if (c!=0){
						if (worm_learner.register_main_window_key_press(c,
							Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R),
							Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R),
							Fl::event_key(FL_Alt_L) || Fl::event_key(FL_Alt_R)
						)) {
							Fl::release();
							demand_window_redraw_from_main_thread();
							return 1;

						}
					}
					Fl::release();
				//}
				break;

			}
			case FL_DND_ENTER:
			case FL_DND_RELEASE:
			case FL_DND_LEAVE:
			case FL_DND_DRAG:
				return 1;
			case FL_PASTE:
				ns_handle_drag_and_drop();
				report_changes_made_to_screen();
				return 1;
		
		}
	//	dialog_window->handle(state);
		//cerr << "Could not handle " << state << "\n"; 
		return Fl_Window::handle(state);
	}
};


// APP WINDOW CLASS
class ns_worm_terminal_worm_window : public Fl_Window {
	bool have_focus;
public:
    ns_worm_gl_window * gl_window;
	ns_death_event_solo_annotation_group * annotation_group;
	
	static unsigned long info_bar_height(){return 18;}
	/*
	static unsigned long expersent_bar_width(){return 145;}
	static unsigned long region_name_bar_width(){return 50;}
	static unsigned long strain_bar_width(){return 140;}
	*/
	static unsigned long border_height() {return 0;}
	static unsigned long border_width() {return 0;}
	~ns_worm_terminal_worm_window (){
		delete gl_window;
		delete annotation_group;
	}

	void update_information_bar(const std::string & status){
		annotation_group->info_bar->value(status.c_str());
	}
    ns_worm_terminal_worm_window (int W,int H,const char*L=0) : Fl_Window(W,H,L), have_focus(false){
        // OpenGL window
		begin();
		
		ns_vector_2i size(worm_learner.worm_window.gl_image_size.x, worm_learner.worm_window.gl_image_size.y + image_window_size_difference(worm_learner.worm_window.display_rescale_factor).y);
	
        gl_window = new  ns_worm_gl_window(30, 30, size.x,size.y);
		
		annotation_group = new ns_death_event_solo_annotation_group(0,
															  worm_learner.worm_window.gl_image_size.y,
															  W,
															  ns_death_event_solo_annotation_group::button_height);
		resize(0, 0, 0, 0);
		//annotation_group->deactivate();
		//annotation_group->hide();
		
	    end();
		gl_window->show();
		

	//	ask_modal_question("Hi","1","2","3");
    }
	
	static ns_vector_2i image_window_size_difference(const float menu_d){
		return ns_vector_2i(0,(long)menu_d*ns_death_event_solo_annotation_group::button_height);
	}
	void size(int w__, int h__) { ns_worm_terminal_worm_window::resize(x(), y(), w__, h__); }
	void resize(int w__, int h__) {
		ns_worm_terminal_worm_window::resize(x(), y(), w__, h__);
	}
  void get_window_size_needed(int & w, int & h, float & d, float & menu_d){
    d = worm_learner.worm_window.display_rescale_factor;
    menu_d = worm_learner.worm_window.display_rescale_factor;
    if (!SCALE_FONTS_WITH_WINDOW_SIZE)
      menu_d = 1;
    
    //	worm_learner.main_window.image_size = ns_vector_2d(w_,h_);
    //	worm_learner.main_window.specified_gl_image_size = (worm_learner.main_window.image_size+
    //			image_window_size_difference());
    
    w = worm_learner.worm_window.gl_image_size.x*d,
      h = worm_learner.worm_window.gl_image_size.y*d+image_window_size_difference(menu_d).y;
//	cerr << w << "," << h << "\n";
    if (w == 0)
      w = 600;
    if (h == 0)
      h = 800;
    
  }
	void resize(int x, int y, int w_requested, int h_requested){

	//	ns_acquire_lock_for_scope lock(worm_learner.worm_window.display_lock, __FILE__, __LINE__);
	
		ns_vector_2i window_size;
		float d,menu_d;
		get_window_size_needed(window_size.x,window_size.y,d,menu_d);


		Fl_Window::resize(x, y, window_size.x, window_size.y);

		gl_window->resize(0,0, worm_learner.worm_window.gl_image_size.x*d, worm_learner.worm_window.gl_image_size.y*d);

		gl_window->redraw();
		
		ns_vector_2i bar_pos(0, worm_learner.worm_window.gl_image_size.y*d), 
			bar_size(window_size.x, ns_death_event_solo_annotation_group::button_height*menu_d);
		
		annotation_group->resize(bar_pos.x,bar_pos.y, bar_size.x, bar_size.y);

	//	lock.release();
	}
	private:
		
		
	int handle(int state) {
		if (debug_handlers) cout << "w";

		switch(state){ 
			case FL_FOCUS:
				have_focus = true;
				break;
			case FL_UNFOCUS:
				have_focus = false;
				break;
			case FL_KEYDOWN:{
				//if (have_focus){
					Fl::lock();
					int c(Fl::event_key());
					Fl::release();
					if (c!=0){
						if (worm_learner.register_worm_window_key_press(c,
							Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R),
							Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R),
							Fl::event_key(FL_Alt_L) || Fl::event_key(FL_Alt_R)
						)) {
							worm_learner.death_time_solo_annotater.request_refresh();
							report_changes_made_to_screen();

							return 1;
						}
					}
				//}
				break;
			}
		
		}
	//	dialog_window->handle(state);
		//cerr << "Could not handle " << state << "\n"; 
		return Fl_Window::handle(state);
	}
};


void ns_specify_worm_details(const ns_64_bit region_id,const ns_stationary_path_id & worm, const ns_death_time_annotation & sticky_properties, std::vector<ns_death_time_annotation> & event_times,double external_rescale_factor){
	worm_learner.storyboard_annotater.specifiy_worm_details(region_id,worm,sticky_properties,event_times);
	worm_learner.storyboard_annotater.redraw_current_metadata(external_rescale_factor);
	worm_learner.storyboard_annotater.request_refresh();
}


struct ns_asynch_annotation_saver {
	static ns_thread_return_type run_asynch(void * l) {
		ns_asynch_annotation_saver launcher;
		launcher.launch();
		return 0;
	}
	void launch() {
		try {
			ns_set_menu_bar_activity(false);
			worm_learner.navigate_death_time_annotation(ns_image_series_annotater::ns_save);
			ns_set_menu_bar_activity(true);
		}
		catch (ns_ex & ex) {
			cerr << "Error loading worm info:" << ex.text();
			worm_learner.death_time_solo_annotater.close_worm();
			show_worm_window = false;
			ns_set_menu_bar_activity(true);
		}
	}
};

void ns_handle_death_time_annotation_button(Fl_Widget * w, void * data){
	ns_image_series_annotater::ns_image_series_annotater_action action(*static_cast<ns_image_series_annotater::ns_image_series_annotater_action *>(data));
	if (action == ns_image_series_annotater::ns_save){
		ns_thread t(ns_asynch_annotation_saver::run_asynch,0);
		t.detach();
		return;
	}
	worm_learner.navigate_death_time_annotation(action);
}
void ns_handle_death_time_solo_annotation_button(Fl_Widget * w, void * data){
	
	ns_death_time_solo_posture_annotater::ns_image_series_annotater_action * a = static_cast<ns_death_time_solo_posture_annotater::ns_image_series_annotater_action *>(data);
	ns_death_time_solo_posture_annotater::ns_image_series_annotater_action action(*a);
	//delete a;
	worm_learner.navigate_solo_worm_annotation(action);
	worm_learner.storyboard_annotater.request_refresh();
	worm_learner.death_time_solo_annotater.request_refresh();
	report_changes_made_to_screen();
//	cerr << "WHA";
	//Fl::focus(current_window->gl_window);
}

void all_menu_callback(Fl_Widget*w, void*data) {
 	ns_menu_organizer_callback_data  ww (*static_cast<ns_menu_organizer_callback_data *>(data));
	char pn[512];
	
	int ret(ww.bar->item_pathname(pn,512));
	if (ret == 0) ww.organizer->dispatch_request_asynch(pn);
	else if (ret == -1) throw ns_ex("Could not identify requested menu item");
	else if (ret == -2) throw ns_ex("Menu item is larger than buffer provided");
	else throw ns_ex("FLTK returned a cryptic error while processing menu request");
}
/*
void redraw_screen(){
	redraw_screen(0,0,false);
}*/

Fl_Menu_Bar * get_menu_bar(){
	return current_window->main_menu;
}
ns_worm_terminal_main_menu_organizer * get_menu_handler(){
	return current_window->main_menu_handler;
}
void update_region_choice_menu(){
	current_window->update_region_choice_menu();
}
void update_strain_choice_menu(){
	current_window->update_strain_choice_menu();
}

void update_exclusion_choice_menu(){
	current_window->update_exclusion_choice_menu();
}

void ns_update_main_information_bar(const std::string & status){
	cerr << status;
	current_window->update_information_bar(status);
}
void ns_update_worm_information_bar(const std::string & status) {
//	cerr << status;
	worm_window->update_information_bar(status);
}

void redraw_main_window(){
  //cerr << "req" << w << "," << h << "\n";
	worm_learner.main_window.redraw_requested = false;

	ns_acquire_lock_for_scope lock(worm_learner.main_window.display_lock, __FILE__, __LINE__);
	ns_vector_2i window_size;
	float d, menu_d;
	current_window->get_window_size_needed(window_size.x, window_size.y, d, menu_d);
	lock.release();
	//Fl::awake();
	Fl::lock();
	if (abs(window_size.x-current_window->w())>=2 || abs(window_size.y - current_window->h()) >= 2){
	  current_window->resize(current_window->x(),current_window->y(),window_size.x,window_size.y);	
	}
	current_window->gl_window->damage(1);
	current_window->gl_window->redraw();
	//Fl::check();
	Fl::unlock();
}


void redraw_worm_window(){
	worm_learner.worm_window.redraw_requested = false;
	ns_acquire_lock_for_scope lock(worm_learner.worm_window.display_lock, __FILE__, __LINE__);
	ns_vector_2i window_size;
	float d, menu_d;
	worm_window->get_window_size_needed(window_size.x, window_size.y, d, menu_d);
	lock.release();
	Fl::lock();
	if (abs(window_size.x - worm_window->w()) >= 2 || abs(window_size.y - worm_window->h()) >= 2){
	//	cerr << "rww(" << h << ")\n";
		worm_window->resize(worm_window->x(),worm_window->y(), window_size.x, window_size.y);
	}
	worm_window->gl_window->damage(1);
	worm_window->gl_window->redraw();
	//Fl::check();
	Fl::unlock();
}


ns_64_bit init_time;
void ns_show_worm_display_error(){
	cerr << "Error! Could not show image.\n";
}

void schedule_repeating_callback(void *a ) {
	if (a == 0)
		Fl::add_timeout(1.0 / IDLE_THROTTLE_FPS, idle_main_window_update_callback,0);
	else
		Fl::repeat_timeout(1.0 / IDLE_THROTTLE_FPS, idle_main_window_update_callback,0);
}
void ns_handle_menu_bar_activity_request();


ns_lock redraw_rate_limiting_lock("overdisplay");
bool redrawing_rate_limiter = false;

void perform_screen_redraw_callback(void * a) {
  try{
	if (debug_handlers) cout << "D";
	try {
	

		if (worm_learner.main_window.redraw_requested)
			redraw_main_window();

		if (worm_learner.worm_window.redraw_requested) 
			redraw_worm_window();
		
		redrawing_rate_limiter = false;
	}
	catch (...) {

		Fl::unlock();
		redrawing_rate_limiter = false;
		throw;
	}
  }
  catch(ns_ex & ex){
    cout << "redraw callback exception: " << ex.text() << "\n";
  }
  catch(...){
    cout << "Unknown callback exeption\n";
  }
}

void request_rate_limited_window_redraw_from_main_thread() {
	if (redrawing_rate_limiter) {
		if (debug_handlers) cout << "r";
		return;
	}
	if (!redraw_rate_limiting_lock.try_to_acquire(__FILE__, __LINE__)) {
		if (debug_handlers) cout << "r";
		return;
	}
	if (redrawing_rate_limiter) {
		if (debug_handlers) cout << "r";
		return;
	}
	if (debug_handlers) cout << "R";
	redrawing_rate_limiter = true;
	redraw_rate_limiting_lock.release();
	Fl::awake(perform_screen_redraw_callback, 0);
}

void demand_window_redraw_from_main_thread() {
	Fl::awake(perform_screen_redraw_callback, 0);
}

void report_changes_made_to_screen() {
	if (debug_handlers) cout << "z";
	//this will also call idle_window_update_callbacl
	Fl::awake(idle_main_window_update_callback,(void *) 1);

}


ns_64_bit last_callback_time(0);

void idle_main_window_update_callback(void * force_redraw) {
  try{
    /*ns_acquire_lock_for_scope dlock(worm_learner.main_window.display_lock,__FILE__,__LINE__);
    ns_vector_2i main_window_requested_size;
    float d,menu_d;
    current_window->get_window_size_needed(main_window_requested_size.x,main_window_requested_size.y,d,menu_d);

    dlock.release();

    
    const bool main_window_is_wrong_size(current_window->w() != main_window_requested_size.x || current_window->h() != main_window_requested_size.x);
    */
  if (debug_handlers) cout << "i";
	{
	
		bool schedule_timer(false);


		Fl::lock();
		ns_image_series_annotater::ns_image_series_annotater_action a(worm_learner.current_annotater->fast_movement_requested());
		if (a == ns_image_series_annotater::ns_fast_forward ||
			a == ns_image_series_annotater::ns_fast_back ||
			current_window->draw_animation || 
		    current_window->last_draw_animation //||
		    // main_window_is_wrong_size
)
			schedule_timer = true;
		Fl::unlock();

		if (schedule_timer) {
			worm_learner.main_window.redraw_requested = true;
			if (worm_window->visible())
			worm_learner.worm_window.redraw_requested = true;
			Fl::awake(schedule_repeating_callback, (void*)1);// Fl::repeat_timeout(1.0 / IDLE_THROTTLE_FPS, idle_main_window_update_callback);
		}
	}
	//if (idle_rate_limiter)
	//	return;

	/*ns_try_to_acquire_lock_for_scope  idle_lock(idle_rate_limiting_lock);

	if (!idle_lock.try_to_get(__FILE__, __LINE__)) return;
	if (idle_rate_limiter) return;
	idle_rate_limiter = true;
	idle_lock.release();*/
	try {

		ns_64_bit last_time = last_callback_time;
		last_callback_time = GetTime();
		ns_64_bit last_interval = last_callback_time - last_time;

		//always call both functions together
		if (worm_window->visible()) {
			Fl::unlock();
			idle_worm_window_update_callback(force_redraw);
			Fl::lock();
		}
			ns_handle_menu_bar_activity_request();
			if (worm_learner.current_annotater->refresh_requested()) {
				if (debug_handlers) cout << "A";
				worm_learner.current_annotater->display_current_frame();
				worm_learner.main_window.redraw_requested = true;
			}

			ns_image_series_annotater::ns_image_series_annotater_action a(worm_learner.current_annotater->fast_movement_requested());
			if (a == ns_image_series_annotater::ns_fast_forward) {
				worm_learner.current_annotater->step_forward(ns_show_worm_display_error,worm_learner.worm_window.display_rescale_factor);
				worm_learner.current_annotater->display_current_frame();
			}
			else if (a == ns_image_series_annotater::ns_fast_back) {
				worm_learner.current_annotater->step_back(ns_show_worm_display_error, worm_learner.worm_window.display_rescale_factor);
				worm_learner.current_annotater->display_current_frame();
			}
			//draw busy animation if requested
			if (current_window->draw_animation) {
				worm_learner.draw_animation((GetTime() - init_time) / 1000.0);
				current_window->last_draw_animation = true; 
				worm_learner.main_window.redraw_requested = true;
			}
			//clear animation when finished
			if (!current_window->draw_animation && current_window->last_draw_animation) {
				current_window->last_draw_animation = false;
				worm_learner.main_window.redraw_requested = true;
				worm_learner.draw();
			}
			
			
			ns_vector_2i window_size;
			float d, menu_d; 
			if (worm_window->visible() || show_worm_window) {
				ns_acquire_lock_for_scope lock(worm_learner.worm_window.display_lock, __FILE__, __LINE__);
				worm_window->get_window_size_needed(window_size.x, window_size.y, d, menu_d);
				lock.release();
			
				if (abs(worm_window->w() - window_size.x) >= 2 || abs(worm_window->h() - window_size.y) >= 2) {
					ns_vector_2i cur(worm_window->w(), worm_window->h());
				//	cerr << "Cur:" << cur << "; des:" << window_size << "; diff:" << (cur - window_size) << "\n";
					ns_acquire_lock_for_scope lock(worm_learner.worm_window.display_lock, __FILE__, __LINE__);
					worm_window->resize(worm_window->x(), worm_window->y(), window_size.x, window_size.y);
					lock.release();
				}
			}

			if (show_worm_window) {
				ns_acquire_lock_for_scope lock(worm_learner.worm_window.display_lock, __FILE__, __LINE__);
				show_worm_window = false;
				hide_worm_window = false;
				worm_window->get_window_size_needed(window_size.x, window_size.y, d, menu_d);
				worm_window->resize(worm_window->x(), worm_window->y(), window_size.x, window_size.y);
				worm_window->show();
				lock.release();
				ns_set_menu_bar_activity(true);
			}
			if (hide_worm_window) {
				hide_worm_window = false;
				ns_acquire_lock_for_scope lock(worm_learner.worm_window.display_lock, __FILE__, __LINE__);
				worm_window->hide();
				lock.release();
			}
			if (force_redraw)
				demand_window_redraw_from_main_thread();
			else
				request_rate_limited_window_redraw_from_main_thread();
		
			Fl::unlock();
		}
		catch (...) {
			Fl::unlock();
		}
  }
  catch(ns_ex & ex){
    cerr << "Idle Error: " << ex.text() << "\n";
  }
  catch(...){
    cerr << "Unknown idle error\n";
      }
}
void ns_hide_worm_window(){
	hide_worm_window = true;
}

void idle_worm_window_update_callback(void * force_redraw){
	//double last_time = c_time;
	//c_time =  GetTime() - init_time;
	//cerr << "FPS = " << 1.0/(time-last_time) << "\n";
	//Fl::lock();
	try{
		Fl::lock();
		ns_vector_2d cur_size(worm_window->w(),worm_window->h());
		Fl::unlock();
		float menu_d(worm_learner.worm_window.display_rescale_factor);
		if (!SCALE_FONTS_WITH_WINDOW_SIZE)
			menu_d = 1;
		cur_size = cur_size -worm_image_window_size_difference()*menu_d;
		cur_size = cur_size/worm_learner.worm_window.display_rescale_factor;

		bool something_done(false);
	
		ns_image_series_annotater::ns_image_series_annotater_action a(worm_learner.death_time_solo_annotater.fast_movement_requested());
		if (a == ns_image_series_annotater::ns_fast_forward){
			worm_learner.death_time_solo_annotater.step_forward(ns_hide_worm_window, worm_learner.worm_window.display_rescale_factor);
			worm_learner.death_time_solo_annotater.display_current_frame();
			something_done = true;
		}
		else if (a==ns_image_series_annotater::ns_fast_back){
			worm_learner.death_time_solo_annotater.step_back(ns_hide_worm_window, worm_learner.worm_window.display_rescale_factor);
			worm_learner.death_time_solo_annotater.display_current_frame();
			something_done = true;
		}
		else if (worm_learner.death_time_solo_annotater.refresh_requested(), worm_learner.worm_window.display_rescale_factor) {
				worm_learner.death_time_solo_annotater.display_current_frame();
				something_done = true;
			}

		if (force_redraw)
			demand_window_redraw_from_main_thread();
		if (something_done){
			request_rate_limited_window_redraw_from_main_thread();
		}
	}
	catch(...){
	//	Fl::unlock();
	}
}



void ns_transfer_annotations(const std::string & annotation_source_filename, const std::string & annotation_destination_filename, const std::string & output_filename){
	
	ns_image_standard annotation_source, destination_image;

	ns_load_image(annotation_destination_filename,destination_image);
	ns_load_image(annotation_source_filename,annotation_source);
	//if (metadata_source_filename == "Y:\\image_server_storage\\partition_000\\2010_04_17_daf16_split_3\\training_set\\2010_04_17_daf16_split_3=163=ben_d=2506=1271547790=2010-04-17=19-43=250429=8031647=2=648488.tif"){
	//	cerr << "ok";
	//}
	ns_image_standard output;
	cerr << "Processing " << output_filename << "\n";
	try{
		ns_transfer_annotations(annotation_source, destination_image,output);
		ns_save_image(output_filename,output);
	}
	catch(ns_ex & ex){
		cerr << "Error: " << ex.text() << "\n";
	}

	/*
	if (file_source.properties().width != destination.properties().width ||
		file_source.properties().height != destination.properties().height)
		throw ns_ex("Image Size Mismatch: Source::") << file_source.properties().width << "x" << file_source.properties().height <<
		" vs " << destination.properties().width << "x" << destination.properties().height
		<< "\nMetadata: " << metadata_source_filename << "\n Image data " << destination_filename << "\n";
	*/

	//ns_save_image(destination_filename,annotated_image);
}

void ns_transfer_annotations_directory(const std::string & annotation_source_directory, const std::string & annotation_destination_directory, const std::string & output_destination_directory){
	ns_dir source_dir,
		   destination_dir;
	vector<std::string> source_files,
						destination_files;
	source_dir.load_masked(annotation_source_directory,"tif",source_files);
	destination_dir.load_masked(annotation_destination_directory,"tif",destination_files);
	set<string> destinations;

	for (unsigned int i = 0; i < destination_files.size(); i++)
		destinations.insert(destination_files[i]);
	
	for (unsigned int i = 0; i < source_files.size(); i++){
		set<string>::iterator p = destinations.find(source_files[i]);
		if (p == destinations.end())
			throw ns_ex("Could not find metadata source for ") << destination_files[i];
		cerr << i << "/" << destination_files.size() << "\n";
		ns_dir::create_directory_recursive(output_destination_directory);
		ns_transfer_annotations(annotation_source_directory + DIR_CHAR_STR + source_files[i],annotation_destination_directory + DIR_CHAR_STR + *p, output_destination_directory + DIR_CHAR_STR + *p);
	}
}



void ns_update_sample_info(ns_sql & sql){
	sql << "SELECT id, device_name, experiment_id FROM capture_samples WHERE device_capture_period_in_seconds = 0 || number_of_consecutive_captures_per_sample > 10";
	ns_sql_result res;
	sql.get_rows(res);	
	ns_sql_result res2;
	for (unsigned int i = 0; i < res.size(); i++){	
		sql << "SELECT count(*) FROM capture_samples WHERE device_name = '" << res[i][1] << "' AND experiment_id = " << res[i][2];
		sql.get_rows(res2);
		const unsigned long number_of_samples(atol(res2[0][0].c_str()));
	
		sql << "SELECT scheduled_time FROM capture_schedule WHERE sample_id = " << res[i][0] << " ORDER BY scheduled_time ASC LIMIT 20";
		sql.get_rows(res2);
		unsigned long short_i(10000000);
		unsigned long long_i(0);
		for (unsigned int j = 1; j < res2.size(); j++){
			unsigned long d(atol(res2[j][0].c_str())-atol(res2[j-1][0].c_str()));
			if (d < short_i)
				short_i = d;
			if (d > long_i)
				long_i = d;
		}
		cerr << "short: " << short_i/60 << ",long: " << long_i/60;
		double r;
		unsigned long p;
		if (long_i == short_i){
			p = short_i/number_of_samples;
			r = 1;
		}
		else{
			p = short_i;
			r = (long_i-short_i)/(double)(short_i*(number_of_samples-1));
			if (floor(r) != r){
				cerr << "Weird intervals!\n";
				continue;
			}
		}

		cerr << ", repeats: " << r << "\n";
		sql << "UPDATE capture_samples SET device_capture_period_in_seconds = "
			<< p
			<<", number_of_consecutive_captures_per_sample = "
			<< r 
			<< " WHERE id = " << res[i][0];
		sql.send_query();

	}

};
void refresh_main_window_internal(void *) {
	current_window->redraw();
}
void refresh_main_window(){
	current_window->redraw();
}
#include <FL/Fl_File_Icon.H>
// MAIN
#include <assert.h>
int main() {
	

	ns_worm_browser_output_debug(__LINE__,__FILE__,"Launching worm browser");
	init_time = GetTime();
	Fl::lock();
	Fl_File_Icon::load_system_icons();
	Fl::scheme("none");

	worm_learner.worm_window.dynamic_range_rescale_factor = 1.75;
	worm_learner.main_window.dynamic_range_rescale_factor = 1;
	//never used?
	//ns_main_thread_id = GetCurrentThread();
	try{
	
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Loading constants");
		image_server.load_constants(ns_image_server::ns_worm_terminal_type); 
		if (image_server.verbose_debug_output())
			output_debug_messages = true;
		

		worm_learner.maximum_window_size = image_server.max_terminal_window_size;
		worm_learner.death_time_annotater.set_resize_factor(image_server.terminal_hand_annotation_resize_factor);
		worm_learner.main_window.display_rescale_factor = image_server.terminal_window_scale_factor();
		worm_learner.worm_window.display_rescale_factor = image_server.terminal_window_scale_factor() / ns_death_time_solo_posture_annotater_timepoint::ns_resolution_increase_factor ;

		//ns_update_sample_info(sql());
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Checking for new release");
		ns_alert_dialog d;
		d.text = "This version of the Worm Browser is outdated.  Please update it.";
		if (image_server.new_software_release_available())
			d.act();
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Loading detection models");
		image_server.load_all_worm_detection_models(worm_learner.model_specifications);
		{
		//	image_server.set_sql_database("image_server_archive");
			
			ns_worm_browser_output_debug(__LINE__,__FILE__,"Getting flags from db");
			ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));
			ns_death_time_annotation_flag::get_flags_from_db(&sql());

	
			ns_worm_browser_output_debug(__LINE__,__FILE__,"Refreshing partition cache");
			image_server.image_storage.refresh_experiment_partition_cache(&sql());
			
			ns_worm_browser_output_debug(__LINE__,__FILE__,"Getting flags from db again");
			ns_death_time_annotation_flag::get_flags_from_db(&sql());

			cerr << "Loading the experiment list...";
			worm_learner.data_selector.load_experiment_names(sql());
			worm_learner.load_databases(sql());
			try{
					
				ns_worm_browser_output_debug(__LINE__,__FILE__,"Setting current experiment");
				worm_learner.data_selector.set_current_experiment(-1,sql());
			}
			catch(ns_ex & ex){
				ns_worm_browser_output_debug(__LINE__,__FILE__,std::string("Error setting experiment: ") + ex.text());
		
				cerr << ex.text() << "\n";
			}
			cerr << "Done.\n";
	
			sql.release();
		}
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Loading for models");
		if (worm_learner.model_specifications.size() == 0){
			cerr << "No model specifications were found in the default model directory.  Worm detection will probably not work.";
			worm_learner.set_svm_model_specification(worm_learner.default_model);
		}
		else {
			
			ns_worm_browser_output_debug(__LINE__,__FILE__,"Setting svm model specification");
			worm_learner.set_svm_model_specification(worm_learner.model_specifications[0]);
		}
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Displaying splash image");
		ns_image_standard im;
		worm_learner.display_splash_image();
		
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Creating new window");

		ns_acquire_lock_for_scope dlock(worm_learner.main_window.display_lock,__FILE__,__LINE__);
		ns_vector_2i main_window_requested_size;
		float dd,menu_d;
		
		dlock.release();

		current_window = new ns_worm_terminal_main_window(1000,1000, "Worm Browser");
		
		current_window->get_window_size_needed(main_window_requested_size.x,main_window_requested_size.y,dd,menu_d);
		current_window->size(main_window_requested_size.x,main_window_requested_size.y);
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Loading occupied animation");
		#ifdef _WIN32
		std::string tmp_filename = "occupied_animation.tif";
		ns_load_image_from_resource(IDR_BIN2,tmp_filename);
		ns_load_image(tmp_filename,worm_learner.animation);
		ns_dir::delete_file(tmp_filename);
		#else
		// note: using implicit string-literal concatenation after preprocessor substitution of NS_DATA_PATH
		ns_load_image(NS_DATA_PATH "occupied_image.tif",worm_learner.animation);
		#endif
		//win.draw_animation = true;
		
		//worm_learner.compare_machine_and_by_hand_annotations();

		//current_window->resizable(win);
		cout << "Compilation date: " << __DATE__ << "\n";
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Showing window");
		current_window->show();
		worm_window = new ns_worm_terminal_worm_window(100,100,"Inspect Worm");

		//worm_window->show();
		worm_window->hide();

		worm_learner.draw();
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Setting current experiment");
		//ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));
		//worm_learner.data_selector.set_current_experiment(341,sql());
		//worm_learner.data_selector.set_current_experiment(-1,sql());
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Setting default sample and region");
		worm_learner.data_selector.select_default_sample_and_region();
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Updating information bar");
		current_window->update_information_bar();
	
		//worm_learner.data_selector.select_region("hare_a::0");
	//	ns_start_death_time_annotation(ns_worm_learner::ns_annotate_storyboard_experiment);
		/*
		ns_time_path_solution solution;
		ns_64_bit region_id(374);//385);
		image_server.add_subtext_to_current_event("Loading data from disk...", &worm_learner.get_sql_connection());
		solution.load_from_db(region_id, worm_learner.get_sql_connection(), true);
		ns_time_path_image_movement_analyzer analyzer;
		const ns_time_series_denoising_parameters time_series_denoising_parameters(ns_time_series_denoising_parameters::load_from_db(region_id, worm_learner.get_sql_connection()));

		ns_image_server::ns_posture_analysis_model_cache::const_handle_t posture_analysis_model_handle;
		image_server.get_posture_analysis_model_for_region(region_id, posture_analysis_model_handle, worm_learner.get_sql_connection());

		ns_acquire_for_scope<ns_analyzed_image_time_path_death_time_estimator> death_time_estimator(
			ns_get_death_time_estimator_from_posture_analysis_model(posture_analysis_model_handle().model_specification));
		analyzer.load_completed_analysis(region_id, solution, time_series_denoising_parameters, &death_time_estimator(), worm_learner.get_sql_connection());
		death_time_estimator.release();
		ns_region_metadata metadata;
		ns_hand_annotation_loader by_hand_region_annotations;
		metadata = by_hand_region_annotations.load_region_annotations(ns_death_time_annotation_set::ns_censoring_and_movement_transitions, region_id, worm_learner.get_sql_connection());
		analyzer.add_by_hand_annotations(by_hand_region_annotations.annotations);
		image_server.add_subtext_to_current_event("\nGenerating visualization...", &worm_learner.get_sql_connection());
		analyzer.generate_movement_posture_visualizations(false, region_id, solution, worm_learner.get_sql_connection());

		*/
		
		ns_worm_browser_output_debug(__LINE__,__FILE__,"Entering idle loop");


	
		return(Fl::run());
	/*	while( Fl::wait() > 0)
			SleepEx(0,TRUE);
			*/
	}
	catch(ns_ex & ex){
		ns_alert_dialog d;
		d.text = ex.text();
		d.act();
		/*
		MessageBox(
		0,
		ex.text().c_str(),
		"Worm Browser",
		MB_TASKMODAL | MB_ICONEXCLAMATION| MB_DEFBUTTON1 | MB_TOPMOST);*/
	/*
	  	cerr << ex.text() << "\n";
		for (unsigned int i = 0; i < 5; i ++){
			cerr << (5-i) << "...";
			ns_thread::sleep(2);
		}*/

	}
	catch(std::exception & exception){
		ns_ex ex(exception);
		cerr << ex.text() << "\n";
		for (unsigned int i = 0; i < 5; i ++){
			cerr << (5-i) << "...";
			ns_thread::sleep(2);
		}
	}
	
}


//ns_sql * worm_window_sql = 0;
struct ns_asynch_worm_launcher{

	ns_64_bit region_id;
	ns_stationary_path_id worm;
	const ns_experiment_storyboard * storyboard;
	unsigned long current_time;
	bool launched;
	static ns_thread_return_type run_asynch(void * l){
		ns_asynch_worm_launcher * launcher(static_cast<ns_asynch_worm_launcher *>(l));
		launcher->launch();
		delete launcher;
		return 0;
	}
	void launch(){
		try{
			worm_learner.death_time_solo_annotater.load_worm(region_id,worm,current_time,worm_learner.solo_annotation_visualization_type,storyboard,&worm_learner, worm_learner.worm_window.display_rescale_factor);
			if (image_server.verbose_debug_output()) image_server.register_server_event_no_db(ns_image_server_event("Finished loading.  Displaying."));
			worm_learner.death_time_solo_annotater.display_current_frame();
			//lock.release();
			show_worm_window = true;
			worm_learner.worm_launch_finished = true;
		//	cerr << "Add alerg dialog back in!\n";
		}
		catch(ns_ex & ex){
			cerr << "Error loading worm info:" << ex.text();
			worm_learner.death_time_solo_annotater.close_worm();
			show_worm_window = false;
		//	stop_death_time_annotation();
			ns_set_menu_bar_activity(true);
			ns_alert_dialog d;
			d.text =  ex.text();
			d.act();
			cerr << ex.text() << "\n";
			worm_learner.worm_launch_finished = true;
		}
	}
};


void ns_launch_worm_window_for_worm(const ns_64_bit region_id, const ns_stationary_path_id & worm, const unsigned long current_time){
	
	ns_set_menu_bar_activity(false);
	ns_asynch_worm_launcher * launcher(new ns_asynch_worm_launcher);
	launcher->region_id = region_id;
	launcher->worm = worm;
	launcher->current_time = current_time;
	launcher->storyboard = &worm_learner.storyboard_annotater.get_storyboard();
	ns_thread worm_launcher(ns_asynch_worm_launcher::run_asynch,launcher);
	worm_launcher.detach();
}
 
void ns_set_main_window_annotation_controls_activity(const bool active){
	if (active)
		current_window->annotation_group->activate();
	else current_window->annotation_group->deactivate();
}

/*ns_thread_return_type asynch_redisplay(void *){
	//cerr << "x";
	
	worm_learner.redraw_screen();
	return 0;
}*/

bool ns_set_animation_state(bool new_state){
	bool old_state(current_window->draw_animation);
	current_window->draw_animation = new_state;
	return old_state;
}
//typedef enum{ns_none,ns_activate,ns_deactivate} ns_menu_bar_request;

ns_menu_bar_request set_menu_bar_request;
void ns_set_menu_bar_activity_internal(bool activate){
	if (activate)
		current_window->draw_animation = false;
//	Fl::awake();
	Fl::lock();
	if (activate){
		for (unsigned int i = 0; i < current_window->main_menu->size(); i++){
			current_window->main_menu->mode(i,current_window->main_menu->mode(i) &  ~FL_MENU_INACTIVE);
	//		current_window->draw_animation = false;
		}
		current_window->main_menu->activate();
		current_window->gl_window->activate();
		current_window->region_menu->activate();
		current_window->strain_menu->activate();
		current_window->annotation_group->activate();
		worm_window->activate();
		worm_window->gl_window->activate();
		worm_window->annotation_group->activate();
		current_window->main_menu->redraw();
		//worm_window->redraw();
	}
	else {

		current_window->draw_animation = true;
		schedule_repeating_callback(0);

		for (unsigned int i = 0; i < current_window->main_menu->size(); i++){
			current_window->main_menu->mode(i,current_window->main_menu->mode(i) |  FL_MENU_INACTIVE);
		}
		current_window->main_menu->deactivate();
		current_window->gl_window->deactivate();
		current_window->region_menu->deactivate();
		current_window->strain_menu->deactivate();
		worm_window->deactivate();
		worm_window->gl_window->deactivate();
		worm_window->annotation_group->deactivate();
		current_window->annotation_group->deactivate();
	//	worm_window->redraw();
		current_window->main_menu->redraw();
	}
	Fl::unlock();
}
void ns_handle_menu_bar_activity_request(){
	menu_bar_processing_lock.wait_to_acquire(__FILE__,__LINE__);
	if (set_menu_bar_request == ns_none){
		menu_bar_processing_lock.release();
		return;
	}
	ns_set_menu_bar_activity_internal(set_menu_bar_request==ns_activate);
	set_menu_bar_request = ns_menu_bar_request::ns_none;
	menu_bar_processing_lock.release();
}
void ns_set_menu_bar_activity(bool activate){
	menu_bar_processing_lock.wait_to_acquire(__FILE__,__LINE__);
	set_menu_bar_request = activate?ns_activate:ns_deactivate;
	menu_bar_processing_lock.release();
	report_changes_made_to_screen();
}
ns_experiment_capture_specification::ns_handle_existing_experiment ask_if_existing_experiment_should_be_overwritten() {
	ns_choice_dialog c;
	c.title = "An an experiment already exists with that name.  What do you want to do?";
	c.option_1 = "Extend the current experiment";
	c.option_2 = "Overwrite the current experiment";
	c.option_3 = "Cancel (do nothing)";
	ns_run_in_main_thread<ns_choice_dialog> b(&c);
	switch (c.result) {
		case 1:
			return ns_experiment_capture_specification::ns_append;
		case 2:
			return ns_experiment_capture_specification::ns_overwrite;
		case 3: return ns_experiment_capture_specification::ns_stop;
		default: throw ns_ex("Unknown option!");
	}
}
void ask_if_schedule_should_be_submitted_to_db(bool & write_to_disk, bool & write_to_db){
	ns_choice_dialog c;
	c.title = "What do you want to do with this experiment schedule?";
	c.option_1 = "Generate a Summary File";
	c.option_2 = "Run it!";
	c.option_3 = "Cancel";
	ns_run_in_main_thread<ns_choice_dialog> b(&c);
	write_to_disk = c.result == 1;
	write_to_db = c.result == 2;
	return;
}

void ns_worm_terminal_main_menu_organizer::show_worm(const std::string & data) {
	worm_window->show();
}
 void ns_experiment_storyboard_annotater_timepoint::load_image(const unsigned long bottom_height,ns_annotater_image_buffer_entry & im,ns_sql & sql,ns_image_standard & temp_buffer,const unsigned long resize_factor_){
	this->blacked_out_non_subject_animals = false;
	if (division != 0){
		for (unsigned int i = 0; i < this->division->events.size(); i++){
			this->division->events[i].drawn_worm_bottom_right_overlay = false;
			this->division->events[i].drawn_worm_top_right_overlay = false;
		}
	}
	experiment_annotater->populate_division_images_from_composit(division_id,sql);
	ns_image_properties prop(division_image.properties());
	prop.components = 3;
	im.im->prepare_to_recieve_image(prop);
	if(division_image.properties().components == 1){
	for (unsigned int y = 0; y < prop.height; y++)
		for (unsigned int x = 0; x < prop.width; x++){
			(*im.im)[y][3*x+0] = division_image[y][x];
			(*im.im)[y][3*x+1] = division_image[y][x];
			(*im.im)[y][3*x+2] = division_image[y][x];
		}
	}
	else{
		for (unsigned int y = 0; y < prop.height; y++)
			for (unsigned int x = 0; x < 3*prop.width; x++){
				(*im.im)[y][x] = division_image[y][x];
			}
	}
	//division_image.pump(*im.im,1024);
	//ns_annotater_timepoint::load_image(im,sql,temp_buffer,resize_factor_);
		
//		vis_info.from_xml(temp_buffer.properties().description);
//	check_metadata();
	im.loaded = true;
}



 ns_vector_2i main_image_window_size_difference(){
	 return ns_worm_terminal_main_window::image_window_size_difference (worm_learner.main_window.display_rescale_factor);
 }
 
 ns_vector_2i worm_image_window_size_difference(){
	 return ns_worm_terminal_worm_window::image_window_size_difference (worm_learner.main_window.display_rescale_factor);
 }


ns_death_time_posture_solo_annotater_data_cache ns_death_time_solo_posture_annotater::data_cache;

void ns_quit(){
	ns_acquire_lock_for_scope lock(worm_learner.main_window.display_lock,__FILE__,__LINE__);
	exit(0);
}


void ns_worm_terminal_main_menu_organizer::show_extra_menus(const std::string & value) {
	worm_learner.show_testing_menus = !worm_learner.show_testing_menus;


	ns_acquire_lock_for_scope lock(menu_bar_processing_lock, __FILE__, __LINE__);
	current_window->main_menu_handler->redraw_menus(*(current_window->main_menu));
	lock.release();
}
