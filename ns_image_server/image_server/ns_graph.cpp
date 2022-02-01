#include "ns_graph.h"
#include <limits>
#include <cmath>
#include <cfloat>

using namespace std;

#define FREETYPE_SCALE_FACTOR 1
#define MAJOR_TICK_HEIGHT 5
#define MINOR_TICK_HEIGHT 2
using std::numeric_limits;

const bool ns_graph::debug_range_checking = true;

int  ns_graph::check_input_data(){
	//see if an independant variable was specified for all input data
	int global_independant_variable_id = -1;
	for (unsigned int i = 0; i < contents.size(); i++){
		if (contents[i]->type == ns_graph_object::ns_graph_independant_variable){
			if (global_independant_variable_id == -1)
				global_independant_variable_id = i;
			else throw ns_ex("ns_graph::Multiple global independant variables specified!");
		}
	}
	//if an independant variable was specified, disallow scatter data.
	if (global_independant_variable_id != -1){
		for (unsigned int i = 0; i < contents.size(); i++)
			if (contents[i]->type == ns_graph_object::ns_graph_dependant_variable && contents[i]->x.size() != 0)
				throw ns_ex("ns_graph::Cannot mix independant variables and scatter plots.");
	}
	//check to see all scatter data series are valid
	else{
		for (unsigned int i = 0; i < contents.size(); i++)
			if (contents[i]->type == ns_graph_object::ns_graph_dependant_variable && contents[i]->x.size() != contents[i]->y.size())
				throw ns_ex("ns_graph:: Data series ") << contents[i]->data_label << " has unmatched scatter data: size(x,y) = (" << (unsigned int)contents[i]->x.size() << "," << (unsigned int)contents[i]->y.size() << ")";
	}
	return global_independant_variable_id;
}



///calculate some graphing parameters from the data range, image size, and specified image parameters
void ns_graph::calculate_graph_specifics(const unsigned int width, const unsigned int height, ns_graph_specifics & s, const ns_graph_axes & specified_axes){
	ns_graph_axes & axes(s.axes);
	double & dx(s.dx);
	double & dy(s.dy);

	const unsigned int & w(width);
	const unsigned int & h(height);

	//the boundary is calculated based on the size of the text in the margins, in draw()
	const ns_vector_2i& bottom_left_border(s.boundary_bottom_and_left),
		top_right_border(s.boundary_top_and_right);

	if ((bottom_left_border.x+top_right_border.x)+1 >= (long)width || (bottom_left_border.y + top_right_border.y) +1 >= (long)height)
		throw ns_ex("ns_graph::Border (") << bottom_left_border.x << "-" << top_right_border.x << "," << bottom_left_border.y << "-" << top_right_border << ") is too large for graph with the dimentions" << w << "x" << h << "pix";

	const int & global_independant_variable_id(s.global_independant_variable_id);

	axes[0] = axes[2] = FLT_MAX;
	axes[1] = axes[3] = -FLT_MAX;

	axes.axis_offset(0) = specified_axes.axis_offset(0);
	axes.axis_offset(1) = specified_axes.axis_offset(1);

	//calculate bounds on x axis, if not specified 
	if (specified_axes.boundary_specified(0) && specified_axes.boundary_specified(1)){
		axes.boundary(0) = specified_axes.boundary(0);
		axes.boundary(1) = specified_axes.boundary(1);
	}
	else{
		if ( global_independant_variable_id != -1){
			//if there is a global independant x axis
			for (unsigned int i = 0; i < contents[global_independant_variable_id]->x.size(); i++){
				if (debug_range_checking && !std::isfinite(contents[global_independant_variable_id]->x[i]))
					throw ns_ex("Found a non-finite number in the shared x axis: ") << contents[global_independant_variable_id]->x[i];
				if (contents[global_independant_variable_id]->x[i] < axes[0])
					axes[0] = contents[global_independant_variable_id]->x[i];
				if (contents[global_independant_variable_id]->x[i] > axes[1])
					axes[1] = contents[global_independant_variable_id]->x[i];
			}
		}
		else{
			//if using a scatter plot
			for (unsigned int i = 0; i < contents.size(); i++){
				for (unsigned int j = 0; j < contents[i]->x.size(); j++){
					if (debug_range_checking && !std::isfinite(contents[i]->x[j]))
						throw ns_ex("Found a non-finite number in the x axis of object ") << i << ": " << contents[i]->x[j];
					if (contents[i]->x[j] < axes[0])
						axes[0] = contents[i]->x[j];
					if (contents[i]->x[j] > axes[1])
						axes[1] = contents[i]->x[j];
				}
			}
		}
		if (specified_axes.boundary_specified(0)) axes[0] = specified_axes.boundary(0);
		if (specified_axes.boundary_specified(1)) axes[1] = specified_axes.boundary(1);
	}

	//calculate bounds on y axis if not specified
	if (specified_axes.boundary_specified(2) && specified_axes.boundary_specified(3)){
		axes[2] = specified_axes.boundary(2);
		axes[3] = specified_axes.boundary(3);
	}
	else{
		for (unsigned int i = 0; i < contents.size(); i++){
			for (unsigned int j = 0; j < contents[i]->y.size(); j++){
				if (debug_range_checking && !std::isfinite(contents[i]->y[j]))
					throw ns_ex("Found a non-finite number in the y axis of object ") << i << ": " << contents[i]->y[j];
				if (contents[i]->y[j] < 0 && !contents[i]->properties.draw_negatives)
					continue;
				if (contents[i]->y[j] < axes[2])
					axes[2] = contents[i]->y[j];
				if (contents[i]->y[j] > axes[3])
					axes[3] = contents[i]->y[j];
			}
			if (contents[i]->properties.fill_between_y_and_ymin) {
				for (unsigned int j = 0; j < contents[i]->y_min.size(); j++) {
					if (debug_range_checking && !std::isfinite(contents[i]->y[j]))
						throw ns_ex("Found a non-finite number in the y_min axis of object ") << i << ": " << contents[i]->y[j];
					if (contents[i]->y_min[j] < 0 && !contents[i]->properties.draw_negatives)
						continue;
					if (contents[i]->y_min[j] < axes[2])
						axes[2] = contents[i]->y_min[j];
					if (contents[i]->y_min[j] > axes[3])
						axes[3] = contents[i]->y_min[j];
				}
			}
		}
		if (specified_axes.boundary_specified(2)) axes[2] = specified_axes.boundary(2);
		if (specified_axes.boundary_specified(3)) axes[3] = specified_axes.boundary(3);
	}


	if (!std::isfinite(axes[0]) ||
		!std::isfinite(axes[1]) ||
		!std::isfinite(axes[2]) ||
		!std::isfinite(axes[3]))
		throw ns_ex("ns_graph::Invalid graph dimentions produced(") << axes[0] << "," << axes[1] << "," <<axes[2] <<"," << axes[3] << ")";
	//check for axes size sanity
	if (axes[0] > axes[1] || axes[2] > axes[3])
		throw ns_ex("ns_graph::Invalid graph dimentions produced: (") << axes[0] << "," << axes[1] << "," <<axes[2] <<"," << axes[3] << ")";

	if (axes[0] == axes[1]){
		axes[0]--;
		axes[1]++;
	}		
	if (axes[2] == axes[3]){
		axes[2]--;
		axes[3]++;
	}

	dy = (h- (bottom_left_border.y + top_right_border.y)-1)/(axes[3]-axes[2]+1.5*specified_axes.axis_offset(1));
	dx = (w- (bottom_left_border.x + top_right_border.x) -1)/(axes[1]-axes[0]+1.5*specified_axes.axis_offset(0));
	//cerr << "dy = (" << h << "-2*" << border<<"-1) = " << (h-2*border-1) << "/(ceil(" << axes[3] <<"-" << axes[2]<< "))=" << ceil(axes[1]-axes[0]) << " = " << dy;


	s.major_x_tick = (axes[1]-axes[0])/s.number_of_x_major_ticks;
	s.minor_x_tick = s.major_x_tick/(s.number_of_x_minor_ticks/s.number_of_x_major_ticks);
	s.major_y_tick = (axes[3]-axes[2])/s.number_of_y_major_ticks;
	s.minor_y_tick = s.major_y_tick/(s.number_of_y_minor_ticks/s.number_of_y_major_ticks);
	if (specified_axes.tick_interval_specified(0)){
		s.major_x_tick = specified_axes.tick(0);
		s.number_of_x_major_ticks = (unsigned int)((axes[1]-axes[0])/s.major_x_tick);
	}
	if (specified_axes.tick_interval_specified(1)){
		s.minor_x_tick = specified_axes.tick(1);
		s.number_of_x_minor_ticks =  (unsigned int)((axes[1]-axes[0])/s.minor_x_tick);
	}
	if (specified_axes.tick_interval_specified(2)){
		s.major_y_tick = specified_axes.tick(2);
		s.number_of_y_major_ticks =  (unsigned int)((axes[3]-axes[2])/s.major_y_tick);
	}
	if (specified_axes.tick_interval_specified(3)){
		s.minor_y_tick = specified_axes.tick(3);
		s.number_of_y_minor_ticks =  (unsigned int)((axes[3]-axes[2])/s.minor_y_tick);
	}
	bool zero_error(false);
	if (specified_axes.axis_position[0] == ns_graph_axes::ns_at_zero) {
		if ((int)(h - (bottom_left_border.y + top_right_border.y)) < (int)(dy * (0 - axes[2]))) {
			cout << "Cannot place x axis at zero, because of the boundary set at " << axes[2] << "(pixel location: " << dy * axes[2] << "). Placing it at the min value.";
			zero_error = true;
		}
		s.x_axis_pos = (int)(h - bottom_left_border.y) - (int)(dy * (0 - axes[2]));
	}
	if (specified_axes.axis_position[0] == ns_graph_axes::ns_at_min_value || zero_error)
		s.x_axis_pos = (int)(h- bottom_left_border.y);
	else //at max value
		s.x_axis_pos = bottom_left_border.y;
	/*if (!std::isfinite<double>(s.x_axis_pos))
		throw ns_ex("Non-finite x axis position found:\n") <<
		"h:" << h << "\n"
		"border.y:" << border.y << "\n"
		"dy:" << dy << "\n"
		"axes[2]:" << axes[2] << "\n";*/
}
void ns_graph_axes::check_for_sanity() const{
	if (boundary_specified(0) && boundary_specified(1) && boundary(0) >= boundary(1))
		throw ns_ex("Invalid x axis specification: [") <<boundary(0) << "," << boundary(1) << "]";
	if (boundary_specified(2) && boundary_specified(3) && boundary(2) >= boundary(3))
		throw ns_ex("Invalid x axis specification: [") <<boundary(2) << "," << boundary(3) << "]";
}

void ns_graph::draw_legend(const std::string& title, long line_width, bool do_not_resize_image, ns_image_standard& image) {
	unsigned long border = 5;
	unsigned long line_spacing = 5;
	ns_font& font(font_server.get_default_font());
	font.set_height(y_axis_properties.text_size * FREETYPE_SCALE_FACTOR);
	std::map< std::string, ns_color_8> cropped_labels;

	for (unsigned int i = 0; i < contents.size(); i++) {
		if (contents[i]->data_label.empty() || contents[i]->type == ns_graph_object::ns_graph_independant_variable)
			continue;
		std::pair<std::string,ns_color_8> entry(contents[i]->data_label, contents[i]->properties.line.color);
	
		if (entry.first.size() > 26)
			entry.first.resize(26);

		if (!contents[i]->properties.line.draw)
			entry.second = contents[i]->properties.point.color;
		auto p = cropped_labels.find(entry.first);
		if (p == cropped_labels.end())
			cropped_labels.emplace(entry);
		else {
			if (!(p->second == entry.second)) {
				std::cerr << "ns_graph::draw_legend()::Multiple legend entries for " << entry.first << " differ in colors.\n";
			}
		}
	}

	ns_vector_2i largest_dimension;
	{
		ns_font_output_dimension dim(0, 0);
		if (title.size() > 0)
			dim = font.draw_color(0, 0, ns_color_8(0, 0, 0), title, image, false);
		largest_dimension = ns_vector_2i(dim.w, dim.h);
		for (auto p = cropped_labels.begin(); p != cropped_labels.end(); ++p) {
			dim = font.draw_color(0, 0, ns_color_8(0, 0, 0), p->first, image, false);
			if (dim.w > largest_dimension.x)
				largest_dimension.x = dim.w;
			if (dim.h > largest_dimension.y)
				largest_dimension.y = dim.h;
		}
	}
	ns_image_properties prop;
	prop.width = border + line_width + border + largest_dimension.x + border;
	unsigned long number_of_lines = cropped_labels.size() + (title.empty() ? 0 : 1);
	prop.height = border + (largest_dimension.y + line_spacing) * (number_of_lines+1) + border;
	prop.components = 3;
	if (!do_not_resize_image)
		image.resize(prop);
	for (unsigned int y = 0; y < image.properties().height; y++) {
		for (unsigned int x = 0; x < 3 * image.properties().width; x++)
			image[y][x] = 255;
	}
	if (do_not_resize_image) {
		if (prop.height > image.properties().height) {
			font.draw(border, largest_dimension.y, ns_color_8(0, 0, 0), "Could not draw legend (x)", image);
			return;
		}
		if (prop.width > image.properties().width) {
			font.draw(border, largest_dimension.y, ns_color_8(0, 0, 0), "Could not draw legend (x)", image);
			return;
		}
	}

	unsigned long cur_y = border+ (largest_dimension.y + line_spacing)*1;
	const unsigned long text_x(border + line_width + border);
	if (!title.empty()) {
		font.draw_color(text_x, cur_y, ns_color_8(0, 0, 0), title, image);
		cur_y += largest_dimension.y + line_spacing;
	}
	for (auto p = cropped_labels.begin(); p != cropped_labels.end(); ++p) {
		const int line_pos = cur_y - largest_dimension.y / 2;
		image.draw_line_color_thick(ns_vector_2i(border, line_pos), ns_vector_2i(border+line_width, line_pos),p->second,3);
		font.draw_color(text_x, cur_y, ns_color_8(0, 0, 0), p->first, image);
		cur_y += largest_dimension.y + line_spacing;
	}
}
ns_graph_specifics ns_graph::draw(ns_image_standard & image) {
	//check to see if input data isn't self-contradicting.
	//Also, see if a scatter plot is desired (no global independant variable specified)

	x_axis_properties.text_size = 400;
	y_axis_properties.text_size = 400;

	// std::cout << x_axis_properties.text_size << " " << y_axis_properties.text_size << '\n';


	if (image.properties().components != 3)
		throw ns_ex("ns_graph::Graphs must be generated in color.");

	//check for an empty graph
	bool have_contents = false;
	for (unsigned int i = 0; i < contents.size(); i++) {
		if (contents[i]->x.size() != 0 || contents[i]->y.size() != 0) {
			have_contents = true;
			break;
		}
	}
	if (!contains_data()) {
		unsigned int h(image.properties().height),
			w(image.properties().width);
		for (unsigned int _y = 0; _y < h; _y++) {
			for (unsigned int _x = 0; _x < w; _x++) {
				image[_y][3 * _x + 0] = area_properties.area_fill.color.x;
				image[_y][3 * _x + 1] = area_properties.area_fill.color.y;
				image[_y][3 * _x + 2] = area_properties.area_fill.color.z;
			}
		}
		if (title_properties.text.draw) {
			ns_acquire_lock_for_scope font_lock(font_server.default_font_lock, __FILE__, __LINE__);
			font_server.get_default_font().set_height(title_properties.text_size*FREETYPE_SCALE_FACTOR);
			ns_font_output_dimension d =
				font_server.get_default_font().draw_color(0,0, title_properties.text.color, "(No Data Points)", image,false);

			font_server.get_default_font().draw_color((w-d.w)/2, (h - d.h) / 2, title_properties.text.color, "(No Data Points)", image);
			font_lock.release();
		}
		return ns_graph_specifics();
	}

	axes.check_for_sanity();
	const unsigned int h(image.properties().height);
	const unsigned int w(image.properties().width);



	//fill in background
	for (unsigned int _y = 0; _y < h; _y++) {
		for (unsigned int _x = 0; _x < w; _x++) {
			image[_y][3 * _x + 0] = area_properties.area_fill.color.x;
			image[_y][3 * _x + 1] = area_properties.area_fill.color.y;
			image[_y][3 * _x + 2] = area_properties.area_fill.color.z;
		}
	}

	if (!have_contents && area_properties.text.draw) {
		ns_acquire_lock_for_scope font_lock(font_server.default_font_lock, __FILE__, __LINE__);
		font_server.get_default_font().set_height(title_properties.text_size);
		font_server.get_default_font().draw_color(w / 3, 0, title_properties.text.color, title, image);
		font_server.get_default_font().draw_color(w / 3, h / 2, title_properties.text.color, "(no data)", image);
		font_lock.release();
		return ns_graph_specifics();
	}

	ns_graph_specifics spec;
	ns_font_output_dimension x_text_size(0, 0), y_text_size(0, 0);
	if (x_axis_properties.text.draw || y_axis_properties.text.draw) {
		ns_acquire_lock_for_scope font_lock(font_server.default_font_lock, __FILE__, __LINE__);
		if (x_axis_properties.text.draw) {
			font_server.get_default_font().set_height(x_axis_properties.text_size);
			x_text_size = font_server.get_default_font().get_render_size("abcdefghijklmnopqrstuvwxyz1234567890.");
			if (!x_axis_label.empty())
				x_text_size.h = x_text_size.h * 2 + MAJOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor;
		}
		if (y_axis_properties.text.draw) {
			font_server.get_default_font().set_height(y_axis_properties.text_size);
			string longest_axis_label;
			if (y_axis_properties.text_decimal_places == -1)
				longest_axis_label = ns_to_string_short((float)(axes.boundary(3)));
			else longest_axis_label = ns_to_string_short((float)((float)(axes.boundary(3))), y_axis_properties.text_decimal_places);
			y_text_size = font_server.get_default_font().get_render_size(longest_axis_label,ns_pi/2);
			y_text_size.w *= 0.5;
			if (!y_axis_label.empty())
				y_text_size.w = y_text_size.w * 2 + MAJOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor;
		}
		font_lock.release();
	}
	 int padding(200);

	spec.boundary_bottom_and_left.x = (unsigned int)(4*MAJOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor +y_text_size.w);
	spec.boundary_bottom_and_left.y = (unsigned int)(4*MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor +x_text_size.h);
	spec.boundary_top_and_right.x = x_text_size.h/2;  //this needs to be big enough to allow the half of the last x axis label tick to be rendered
	spec.boundary_top_and_right.y = y_text_size.h; //this needs to be big enough to allow the top of the last y axis label tick to be rendered
	

	spec.global_independant_variable_id = check_input_data();
	calculate_graph_specifics(w,h,spec,axes);
	const ns_vector_2i& bottom_left_border(spec.boundary_bottom_and_left),
		top_right_border(spec.boundary_top_and_right);


	//start drawing the graph!
	int line_l_width = x_axis_properties.line.width/2,
		line_r_width = x_axis_properties.line.width/2 + x_axis_properties.line.width%2;
	//draw x axis
	for (unsigned int _x = bottom_left_border.x; _x < w- top_right_border.x; _x++){
		for (int dy = -line_l_width; dy < line_r_width; dy++){
			image[spec.x_axis_pos+dy][3*_x+0] = x_axis_properties.line.color.x;	
			image[spec.x_axis_pos+dy][3*_x+1] = x_axis_properties.line.color.y;	
			image[spec.x_axis_pos+dy][3*_x+2] = x_axis_properties.line.color.z;	
		}
	}


	//draw y axis
	line_l_width = y_axis_properties.line.width/2;
	line_r_width = y_axis_properties.line.width/2 + x_axis_properties.line.width%2;
	for (unsigned int _y = top_right_border.y; _y < h- bottom_left_border.y; _y++){
		for (int dx = -line_l_width; dx < line_r_width; dx++){
			image[_y][3*(bottom_left_border.x +dx)+0] = y_axis_properties.line.color.x;
			image[_y][3*(bottom_left_border.x +dx)+1] = y_axis_properties.line.color.y;
			image[_y][3*(bottom_left_border.x +dx)+2] = y_axis_properties.line.color.z;
		}
	}

	//draw axes labels
	//x axis
	ns_acquire_lock_for_scope font_lock(font_server.default_font_lock, __FILE__, __LINE__);
	font_server.get_default_font().set_height(x_axis_properties.text_size*FREETYPE_SCALE_FACTOR);
	for (unsigned int i = 0; i <= spec.number_of_x_major_ticks; i++){
		unsigned int x = (bottom_left_border.x+(int)(spec.dx*(spec.major_x_tick*i+spec.axes.axis_offset(0))));

		int y1, y2;
		if (axes.axis_position[1] == ns_graph_axes::ns_at_zero) {
			y1 = spec.x_axis_pos - MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
			y2 = spec.x_axis_pos + MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
		}
		else if (axes.axis_position[1] == ns_graph_axes::ns_at_min_value) {
			y1 = spec.x_axis_pos;
			y2 = spec.x_axis_pos + MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
		}
		else if (axes.axis_position[1] == ns_graph_axes::ns_at_max_value) {
			y1 = spec.x_axis_pos;
			y2 = spec.x_axis_pos - MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
		}

		image.draw_line_color_thick(ns_vector_2i(x,y1),ns_vector_2i(x,y2),x_axis_properties.line.color, x_axis_properties.tick_mark_rescale_factor);
		if (x_axis_properties.text.draw){
			std::string text;
			if (x_axis_properties.text_decimal_places == -1)
				text = ns_to_string_short((float)(spec.major_x_tick*i+spec.axes[0]));
			else text = ns_to_string_short((float)(spec.major_x_tick*i+spec.axes[0]),x_axis_properties.text_decimal_places);
			ns_font_output_dimension d = font_server.get_default_font().get_render_size(text);
			int xp = x - d.w / 2;
			if (xp < 0) xp = 0;
			font_server.get_default_font().draw(xp,
												h- bottom_left_border.y+2*MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor+d.h,
												x_axis_properties.text.color,
												text,
												image);
		}
	}
	// if (x_axis_properties.text.draw) {
	// 	ns_font_output_dimension d = font_server.get_default_font().get_render_size(x_axis_label);
	// 	font_server.get_default_font().draw((image.properties().width-top_right_border.x+bottom_left_border.x-d.w)/2, h - 2*MAJOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor, x_axis_properties.text.color, x_axis_label, image);
	// }
	//y axis
	font_server.get_default_font().set_height(y_axis_properties.text_size*FREETYPE_SCALE_FACTOR);
	for (unsigned int i = 0; i <= spec.number_of_y_major_ticks; i++){
		unsigned int y = h - bottom_left_border.y -(int)(spec.dy*(spec.major_y_tick*i+spec.axes.axis_offset(1)));

		image.draw_line_color_thick(ns_vector_2i(bottom_left_border.x -2*MAJOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor,y),ns_vector_2i(bottom_left_border.x,y),y_axis_properties.line.color, y_axis_properties.tick_mark_rescale_factor);
		if (y_axis_properties.text.draw){
			std::string text;
			if (y_axis_properties.text_decimal_places == -1)
				text = ns_to_string_short((float)(spec.major_y_tick*i+spec.axes[2]));
			else text = ns_to_string_short((float)((float)(spec.major_y_tick*i+spec.axes[2])),y_axis_properties.text_decimal_places);
			ns_font_output_dimension d = font_server.get_default_font().get_render_size(text);
			int yp = y + d.h / 2;
			//if (yp < 0) yp = 0;
			font_server.get_default_font().draw(
				(bottom_left_border.x -d.w- 3*MAJOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor) -50,
				yp,
				y_axis_properties.text.color,
				text,
				image);
		}
	}

	 //y axis height (h-2*border.y)
	// if (y_axis_properties.text.draw) {
	// 	ns_font_output_dimension d = font_server.get_default_font().get_render_size(y_axis_label, ns_pi / 2);
	// 	const int y_pos = (h + top_right_border.y - bottom_left_border.y  - d.h)/2;
	// 	font_server.get_default_font().draw(MAJOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor, y_pos, ns_pi / 2, y_axis_properties.text.color, y_axis_label, image);
	// }
	font_lock.release();
	//draw minor ticks
	if (x_axis_properties.draw_tick_marks){
		//x axis
		for (unsigned int i = 0; i <= spec.number_of_x_minor_ticks; i++){
			int x = bottom_left_border.x+(int)(spec.dx*(spec.minor_x_tick*i+spec.axes.axis_offset(0)));
			if (x < 0 || x >= w)
				throw ns_ex("ns_graph::Error drawing x axis minor ticks");
			int y1, y2;
			if (axes.axis_position[1] == ns_graph_axes::ns_at_zero) {
				y1 = spec.x_axis_pos - MINOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
				y2 = spec.x_axis_pos + MINOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
			}
			else if (axes.axis_position[1] == ns_graph_axes::ns_at_min_value) {
				y1 = spec.x_axis_pos;
				y2 = spec.x_axis_pos + MINOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
			}
			else if (axes.axis_position[1] == ns_graph_axes::ns_at_max_value) {
				y1 = spec.x_axis_pos;
				y2 = spec.x_axis_pos - MINOR_TICK_HEIGHT*x_axis_properties.tick_mark_rescale_factor;
			}
			else {
				y1 = spec.x_axis_pos - MINOR_TICK_HEIGHT * x_axis_properties.tick_mark_rescale_factor;
				y2 = spec.x_axis_pos + MINOR_TICK_HEIGHT * x_axis_properties.tick_mark_rescale_factor;
			}
				
			image.draw_line_color(ns_vector_2i(x, y1),ns_vector_2i(x,y2),x_axis_properties.line.color);
		}
	}
	if (y_axis_properties.draw_tick_marks){
		//y axis
		for (unsigned int i = 0; i<= spec.number_of_y_minor_ticks;i++){
			int y = h - bottom_left_border.y -(int)(spec.dy*(spec.minor_y_tick*i + spec.axes.axis_offset(1)));
			if (y < 0 || y >= h)
				throw ns_ex("ns_graph::Error drawing y axis minor ticks");

			image.draw_line_color_thick(ns_vector_2i(bottom_left_border.x-MINOR_TICK_HEIGHT*y_axis_properties.tick_mark_rescale_factor,y),ns_vector_2i(bottom_left_border.x,y),y_axis_properties.line.color,y_axis_properties.tick_mark_rescale_factor);
		}
	}

	//plot data
	if (spec.global_independant_variable_id != -1){
		//not a scatter plot
		for (unsigned int i = 0; i < contents.size(); i++){
			if (i == spec.global_independant_variable_id)
				continue;
			plot_object(*contents[i],*contents[spec.global_independant_variable_id],image,spec);
		}
	}
	//a scatter plot
	else{
		for (unsigned int i = 0; i < contents.size(); i++)
			plot_object(*contents[i],*contents[i],image,spec);
	}
	if (title_properties.text.draw){
		ns_acquire_lock_for_scope font_lock(font_server.default_font_lock, __FILE__, __LINE__);
		font_server.get_default_font().set_height(title_properties.text_size*FREETYPE_SCALE_FACTOR);
		font_server.get_default_font().draw_color(w/3,(3* top_right_border.y)/4,title_properties.text.color,title,image);
		font_lock.release();
	}
	return spec;
}

void ns_graph::draw(ns_svg & svg){
	//check to see if input data isn't self-contradicting.
	//Also, see if a scatter plot is desired (no global independant variable specified)
	
	if (!contains_data()){
		svg.draw_text("(No Data)",ns_vector_2d(20,50),ns_color_8(0,0,0),12,0);
		return;
	}

	axes.check_for_sanity();
	const unsigned int h((unsigned int)(100));
	const unsigned int w((unsigned int)(100*aspect_ratio));

	ns_svg_header_spec sp;
	sp.description = title;
	sp.height=100;
	sp.width = 100;
	sp.percent = true;
	sp.view_box.x = w;
	sp.view_box.y = h;

	svg.specifiy_header(sp);

	//check for an empty graph
	bool have_contents = false;
	for (unsigned int i = 0; i < contents.size(); i++){
		if (contents[i]->x.size() != 0 || contents[i]->y.size() != 0){
			have_contents = true;
			break;
		}
	}
	if (!have_contents){
		svg.draw_text("(no data)",ns_vector_2d(w/3,h/2),area_properties.text.color,(float)1.8);
		return;
	}

	ns_graph_specifics spec;
	spec.aspect_ratio = aspect_ratio;
	spec.boundary_bottom_and_left.x= 7;
	spec.boundary_bottom_and_left.y= 7;
	spec.boundary_top_and_right = spec.boundary_bottom_and_left;
	
	spec.global_independant_variable_id = check_input_data();
	calculate_graph_specifics(w,h,spec,axes);


	//start drawing the graph!

	//fill in background
	svg.draw_rectangle(ns_vector_2d(0,0),ns_vector_2d(w,h),ns_color_8(),area_properties.area_fill.color,1,false,true);


	//draw x axis
	svg.draw_line(ns_vector_2d(spec.boundary_bottom_and_left.x,spec.x_axis_pos),ns_vector_2d(w-spec.boundary_top_and_right.x,spec.x_axis_pos),x_axis_properties.line.color,(float).1);

	//draw y axis
	svg.draw_line(ns_vector_2d(spec.boundary_bottom_and_left.x,spec.boundary_bottom_and_left.y),ns_vector_2d(spec.boundary_bottom_and_left.x,h-spec.boundary_top_and_right.y),x_axis_properties.line.color,(float).1);

	const ns_vector_2i& bottom_left_border = spec.boundary_bottom_and_left,
		top_right_border = spec.boundary_top_and_right;

	//x axis
	for (unsigned int i = 0; i <= spec.number_of_x_major_ticks; i++){
		//major ticks
		double x = bottom_left_border.x + ((spec.dx*spec.major_x_tick*i));
			
		if (x_axis_properties.draw_tick_marks)
		svg.draw_line(ns_vector_2d(x,spec.x_axis_pos-.4),ns_vector_2d(x,spec.x_axis_pos+.4),x_axis_properties.line.color,(float).1);
		//labels
		if (x_axis_properties.text.draw){
			std::string text;
			if (x_axis_properties.text_decimal_places == -1)
				text = ns_to_string_short((float)(spec.major_x_tick*i+spec.axes[0]));
			else text = ns_to_string_short((float)(spec.major_x_tick*i+spec.axes[0]),x_axis_properties.text_decimal_places);
				svg.draw_text(text,ns_vector_2d(x,(float)(h)),x_axis_properties.text.color,(float)x_axis_properties.text_size,(float)-45);
		}
	}

	//y axis
	for (unsigned int i = 0; i <= spec.number_of_y_major_ticks; i++){
		double y = h - bottom_left_border.y -(spec.dy*spec.major_y_tick*i);
		//major ticks
		if (y_axis_properties.draw_tick_marks)
			svg.draw_line(ns_vector_2d(bottom_left_border.x-.4,y),ns_vector_2d(bottom_left_border.x+.4,y),y_axis_properties.line.color,(float).1);
		
		if (y_axis_properties.text.draw){
			std::string text;
			//labels
			if (y_axis_properties.text_decimal_places == -1)
				text = ns_to_string_short((float)(spec.major_y_tick*i+spec.axes[2]));
			else text = ns_to_string_short((float)(spec.major_y_tick*i+spec.axes[2]),y_axis_properties.text_decimal_places);
			svg.draw_text(text,ns_vector_2d(0,y),y_axis_properties.text.color,(float)y_axis_properties.text_size);
		}
	}
	//draw minor ticks
	//x axis
	for (unsigned int i = 0; i <= spec.number_of_x_minor_ticks; i++){
		double x = bottom_left_border.x+(spec.dx*(spec.minor_x_tick*i + spec.axes.axis_offset(0)));
		if (x < 0 || x >= w) throw ns_ex("ns_graph::Error drawing x axis minor ticks");
		svg.draw_line(ns_vector_2d(x,spec.x_axis_pos-.2),ns_vector_2d(x,spec.x_axis_pos+.2),x_axis_properties.line.color,(float).1);
	}
	//y axis
	for (unsigned int i = 0; i<= spec.number_of_y_minor_ticks;i++){
		double y = h - bottom_left_border.y -(spec.dy*(spec.minor_y_tick*i+spec.axes.axis_offset(1)));
		if (y < 0 || y >= h)
			throw ns_ex("ns_graph::Error drawing y axis minor ticks");
		svg.draw_line(ns_vector_2d(bottom_left_border.x-.2,y),ns_vector_2d(bottom_left_border.x+.2,y),y_axis_properties.line.color,(float).1);
	}

	//plot data
	if (spec.global_independant_variable_id != -1){
		//not a scatter plot
		for (unsigned int i = 0; i < contents.size(); i++){
			if (i == spec.global_independant_variable_id)
				continue;
			plot_object(*contents[i],*contents[spec.global_independant_variable_id],svg,spec);
		}
	}
	//a scatter plot
	else{
		for (unsigned int i = 0; i < contents.size(); i++)
			plot_object(*contents[i],*contents[i],svg,spec);
	}
	
	if (title_properties.text.draw)
		svg.draw_text(title,ns_vector_2d(w/3,(3* top_right_border.y)/4),title_properties.text.color,(float)title_properties.text_size);
}


void ns_graph::draw(std::string & svg_output){
	ns_svg svg;
	draw(svg);
	svg.compile(svg_output);
}

void ns_graph::draw(ostream & svg_output){
	ns_svg svg;
	draw(svg);
	svg.compile(svg_output);
}
void ns_graph::plot_object(const ns_graph_object & y, const ns_graph_object & x, ns_image_standard & image, const ns_graph_specifics & spec){
	const ns_graph_axes & axes(spec.axes);
	const ns_vector_2i &border_bl(spec.boundary_bottom_and_left),& border_tr(spec.boundary_top_and_right);
	const double &dx(spec.dx);
	const double &dy(spec.dy);

	if (x.x.empty())
		return;
	if (y.y.size() != x.x.size())
		throw ns_ex("Axis size mismatch");

	for (unsigned int i = 0; i < y.y.size(); i++){
		if (y.y[i] < 0 && !y.properties.draw_negatives)
			continue;
		if (y.y[i] < axes.boundary(2) || y.y[i] > axes.boundary(3))
			throw ns_ex("Data point y=") << (float)y.y[i] << " is outside the specified axis bounds [" << (float)axes.boundary(2) << "," << (float)axes.boundary(3) << "]";
	}
	for (unsigned int i = 0; i < x.x.size(); i++)
		if (x.x[i] < axes.boundary(0) || x.x[i] > axes.boundary(1))
			throw ns_ex("Data point x=") << (float)x.x[i] << " is outside of specified axis bounds [" << (float)axes.boundary(0) << "," << (float)axes.boundary(1) << "]";

	if (y.y.size() == 0)
		return;
	const unsigned int h(image.properties().height),
					   w(image.properties().width);

	if (y.type == ns_graph_object::ns_graph_none || y.type == ns_graph_object::ns_graph_independant_variable)
		throw ns_ex("ns_graph: Attempting to graph invalid graph object");

	else if (y.type == ns_graph_object::ns_graph_dependant_variable){
		//fill area under the curve
		if (y.properties.area_fill.draw && y.y.size() != 0){
			if (y.properties.fill_between_y_and_ymin && y.y_min.size() != y.y.size())
				throw ns_ex("ns_graph()::y and y_min are not the same length.");

			double l = 0,
			r = (axes[1]- axes[0])*dx;
			bool found_index = false;
			for (int pixel_x = l; pixel_x < r; pixel_x++){
				int data_index = 1; // if index < 0 is an error condition below it should be signed

				const double data_x = (pixel_x) / dx + axes[0];
				if (data_x < x.x[0] || data_x > *x.x.rbegin())
					continue;
				if (x.x.size() == 1)
					data_index = 0;
				else {
					switch (y.properties.line_hold_order) {
					case ns_graph_properties::ns_first:
					case ns_graph_properties::ns_zeroth_centered: {
						for (unsigned int i = 1; i < x.x.size(); i++) {
							if (data_x < x.x[i]) {
								if (fabs(x.x[i] + data_x) > fabs(x.x[i - 1] + data_x))
									data_index = i - 1;
								else data_index = i;
								break;
							}
						}
						break;
					}
					case ns_graph_properties::ns_zeroth: {
						for (unsigned int i = 1; i < x.x.size(); i++) {
							if (data_x < x.x[i]) {
								data_index = i - 1;
								break;
							}
						}
						break;
					}
					}
				}
				
				
				//for out-of-bounds locations, draw no data.
				if (data_index < 0 || data_index >= (unsigned int)y.y.size() || data_index >= (unsigned int)x.x.size())
					continue;
				//otherwise draw the data.
				unsigned int pixel_top((int)(h- border_bl.y)-(int)(dy*(y.y[data_index]-axes.boundary(2)+spec.axes.axis_offset(1))));
				if (debug_range_checking && h- border_bl.y < dy*(y.y[data_index]-axes.boundary(2)))
					throw ns_ex("Something went wrong in graph logic.  Graph height: ") << h << ", border: " << border_bl.y <<
					", dy: " << dy << ", boundary(2): " << axes.boundary(2) << " y.y[" << data_index << "]: " << y.y[data_index];
				int pixel_bottom;
				if (!y.properties.fill_between_y_and_ymin)
					pixel_bottom = spec.x_axis_pos + (unsigned int)(spec.dy*spec.axes.axis_offset(1)); // Again if it could be < 0 (below) it needs to be signed.
				else {
					pixel_bottom = (int)(h - border_bl.y) - (int)(dy*(y.y_min[data_index] - axes.boundary(2) + spec.axes.axis_offset(1)));
					if (debug_range_checking && h - border_bl.y < dy*(y.y_min[data_index] - axes.boundary(2)))
						throw ns_ex("Something went wrong in graph logic.  Graph height: ") << h << ", border: " << border_bl.y <<
						", dy: " << dy << ", boundary(2): " << axes.boundary(2) << " y.y[" << data_index << "]: " << y.y[data_index];
				}
				if (pixel_top > pixel_bottom){
					int tmp = pixel_bottom;
					pixel_bottom = pixel_top;
					pixel_top = tmp;
				}
				if (pixel_bottom < 0 && !y.properties.draw_negatives)
					continue; 
				const unsigned long screen_position_x = pixel_x + border_bl.x;
				for (unsigned int _y = pixel_top; _y < pixel_bottom; _y++){
					for (unsigned int c = 0; c < 3; c++) {
						if (debug_range_checking && (_y >= h || (screen_position_x)+c >= w)) {
							ns_ex ex("Graph range upper border error: \n");
							ex <<
								"_y=" << _y << "\n"
								"h=" << h << "\n"
								"pixel_bottom=" << pixel_bottom << "\n"
								"pixel_top=" << pixel_top << "\n"
								"data_index=" << data_index << "\n"
								"y.y.size()=" << y.y.size() << "\n"
								"y.y[data_index]=" << y.y[data_index] << "\n"
								"screen_position_x=" << (screen_position_x) << "\n"
								"c=" << c << "\n"
								"border.y=" << border_bl.y << "\n"
								"dy=" << dy << "\n"
								"y.y_min[data_index]=" << y.y_min[data_index] << "\n"
								"spec.axes.axis_offset(1)=" << spec.axes.axis_offset(1) << "\n"
								"spec.x_axis_pos=" << spec.x_axis_pos << "\n"
								"pixel_x=" << pixel_x << "\n"
								"axes.boundary(0)=" << axes.boundary(0) << "\n"
								"spec.axes.axis_offset(0)=" << spec.axes.axis_offset(0) << "\n"
								"dx=" << dx << "\n"
								"border.x=" << border_bl.x << "\n"
								"axes[3]=" << axes[3] << "\n"
								"axes[2]=" << axes[2] << "\n"
								"axes[1]=" << axes[1] << "\n"
								"axes[0]=" << axes[0] << "\n"
								"axes.axis_offset(1)=" << axes.axis_offset(1) << "\n"
								"axis_offset(0)=" << axes.axis_offset(0) << "\n\n";
								for (unsigned int i = 0; i < y.y.size(); i++) {
									ex << x.x[i] << ", " << y.y[i] << "\n";
								}
								throw ex;
						}


						image[_y][3 * (screen_position_x) + c] = (ns_8_bit)((1.0 - y.properties.area_fill.opacity) * image[_y][3 * (screen_position_x) + c] + y.properties.area_fill.opacity * y.properties.area_fill.color[c]);
					}
				}
			}
/*
			for (unsigned int i = 0; i < x.x.size(); i++){
				unsigned int top = (int)(h-border)-(int)(dy*(y.y[i]-axes.boundary(2)));
				for (unsigned int _y = top; _y < h-border; _y++){
					for (int _x = -dx/2; _x <= dx/2+1; _x++){
						for (unsigned int c = 0; c < 3; c++){
							if (x.x[i] < axes.boundary(0)) continue;
							if (x.x[i] > axes.boundary(1)) continue;
							int rx = 3*(border + (unsigned int)(dx*(x.x[i]-axes.boundary(0))+_x))+c;
							image[_y][rx] = (ns_8_bit)((1.0 - y.properties.area_fill.opacity)*image[_y][rx] + y.properties.area_fill.opacity*y.properties.area_fill.color[c]);
						}
					}
				}
			}*/
		}

		//draw lines
		if (y.properties.line.draw){
			switch(y.properties.line_hold_order){
				case ns_graph_properties::ns_first:{
					for (int i = 0; i < (int)y.y.size()-1; i++){
						if ((y.y[i] < 0 || y.y[i+1] < 0) && !y.properties.draw_negatives)
							continue;
						int x0 = border_bl.x + (unsigned int)(dx*(x.x[i]-axes.boundary(0)+ spec.axes.axis_offset(0))),
							x1 = border_bl.x + (unsigned int)(dx*(x.x[i+1]-axes.boundary(0)+ spec.axes.axis_offset(0))),
							y0 = h- border_bl.y - (unsigned int)(dy*(y.y[i]-axes.boundary(2)+ spec.axes.axis_offset(1))),
							y1 = h- border_bl.y - (unsigned int)(dy*(y.y[i+1]-axes.boundary(2)+ spec.axes.axis_offset(1)));
						if (debug_range_checking) {
							if (x0 < 0 || x0 >= (int)w)
								throw ns_ex("ns_graph::Could not draw line due to x0");
							if (x1 < 0 || x1 >= (int)w)
								throw ns_ex("ns_graph::Could not draw line due to x1");
							if (y0 < 0 || y0 >= (int)h)throw ns_ex("ns_graph::Could not draw line due to y0");
							if (y1 < 0 || y1 >= (int)h)throw ns_ex("ns_graph::Could not draw line due to y1");
						}
						image.draw_line_color_thick(ns_vector_2i(x0,y0),ns_vector_2i(x1,y1),y.properties.line.color,y.properties.line.width);
						if (y.properties.draw_vertical_lines == ns_graph_properties::ns_full_line)
							image.draw_line_color_thick(ns_vector_2i(x0,spec.x_axis_pos),ns_vector_2i(x0,y0),y.properties.line.color,y.properties.line.width);
					}
					break;
				}
				case ns_graph_properties::ns_zeroth:{		
					for (int i = 0; i < (int)y.y.size()-1; i++){
						if ((y.y[i] < 0 || y.y[i+1] < 0) && !y.properties.draw_negatives)
							continue;
						int x0 = border_bl.x + (unsigned int)(dx*(x.x[i]-axes.boundary(0)+ spec.axes.axis_offset(0)))+1,
							x1 = border_bl.x + (unsigned int)(dx*(x.x[i+1]-axes.boundary(0)+ spec.axes.axis_offset(0))),
							y0 = h- border_bl.y - (unsigned int)(dy*(y.y[i]-axes.boundary(2)+ spec.axes.axis_offset(1))),
							y1 = h- border_bl.y - (unsigned int)(dy*(y.y[i+1]-axes.boundary(2)+ spec.axes.axis_offset(1)));
						if (debug_range_checking) {
							if (x0 < 0 || x0 >= (int)w)
								throw ns_ex("ns_graph::Could not draw line due to x0");
							if (x1 < 0 || x1 >= (int)w)
								throw ns_ex("ns_graph::Could not draw line due to x1");
							if (y0 < 0 || y0 >= (int)h)throw ns_ex("ns_graph::Could not draw line due to y0");
							if (y1 < 0 || y1 >= (int)h)throw ns_ex("ns_graph::Could not draw line due to y1");
						}
						int line_connection_offset = 0;
						if (y.properties.draw_vertical_lines == ns_graph_properties::ns_outline || y.properties.draw_vertical_lines == ns_graph_properties::ns_full_line)
							line_connection_offset = y.properties.line.width/2;
						image.draw_line_color_thick(ns_vector_2i(x0- line_connection_offset,y0),ns_vector_2i(x1+line_connection_offset,y0),y.properties.line.color,y.properties.line.width);
						if (y.properties.draw_vertical_lines == ns_graph_properties::ns_full_line){
							image.draw_line_color_thick(ns_vector_2i(x0,spec.x_axis_pos),ns_vector_2i(x0,y0),y.properties.line.color,y.properties.line.width);
							image.draw_line_color_thick(ns_vector_2i(x1,spec.x_axis_pos),ns_vector_2i(x1,y0),y.properties.line.color,y.properties.line.width);
						}
						if (y.properties.draw_vertical_lines == ns_graph_properties::ns_outline){
							image.draw_line_color_thick(ns_vector_2i(x1,y0),ns_vector_2i(x1,y1),y.properties.line.color,y.properties.line.width);
						}
					}
					break;
				}
													
				case ns_graph_properties::ns_zeroth_centered:{
					double d_off(dx*(spec.axes.axis_offset(0)/2));
					if (x.x.size() > 1)
						d_off = (dx*(x.x[1]-x.x[0]))/2.0;
					for (int i = 0; i < (int)y.y.size(); i++){
						if (y.y[i] < 0 && !y.properties.draw_negatives)
							continue;
						double  p0(dx*(x.x[i]-axes.boundary(0)+ spec.axes.axis_offset(0)));
						int x0 = border_bl.x + (unsigned int)(p0-d_off),
							x1 = border_bl.x + (unsigned int)(p0+d_off),
							y0 = h-border_bl.y - (unsigned int)(dy*(y.y[i]-axes.boundary(2)+ spec.axes.axis_offset(1)));
						if (debug_range_checking) {
							if (x0 < 0 || x0 >= (int)w)throw ns_ex("ns_graph::Could not draw line due to x0");
							if (x1 < 0 || x1 >= (int)w)
								throw ns_ex("ns_graph::Could not draw line due to x1");
							if (y0 < 0 || y0 >= (int)h)throw ns_ex("ns_graph::Could not draw line due to y0");
						}
						image.draw_line_color_thick(ns_vector_2i(x0,y0),ns_vector_2i(x1,y0),y.properties.line.color,y.properties.line.width);
						if (y.properties.draw_vertical_lines){
							image.draw_line_color_thick(ns_vector_2i(x0,spec.x_axis_pos),ns_vector_2i(x0,y0),y.properties.line.color,y.properties.line.width);
							image.draw_line_color_thick(ns_vector_2i(x1,spec.x_axis_pos),ns_vector_2i(x1,y0),y.properties.line.color,y.properties.line.width);
						}
					}
					break;
				 }
			}
		}
		//draw point
		if (y.properties.point.draw){
			switch(y.properties.point.point_shape){
				case ns_graph_color_set::ns_square:{
					int point_half_width= y.properties.point.width/2;
					int edge_width= y.properties.point.edge_width;
					for (unsigned int i = 0; i < y.y.size(); i++){
						if (y.y[i] < 0 && !y.properties.draw_negatives)
							continue;
						for (int shape_y = -point_half_width - edge_width; shape_y <= point_half_width + edge_width; shape_y++){
							for (int shape_x = -point_half_width -edge_width; shape_x <= point_half_width + edge_width; shape_x++){
								const ns_color_8 * color(&y.properties.point.edge_color);
								if (shape_y >= -point_half_width && shape_y <= point_half_width &&
									shape_x >= -point_half_width && shape_x <= point_half_width)
									color = &y.properties.point.color;
								for (unsigned int c = 0; c < 3; c++){
									int rx = 3*(border_bl.x + (int)(dx*(x.x[i] - axes.boundary(0)+ spec.axes.axis_offset(0)) + shape_x))+c,
										ry = (int)(h-border_bl.y)-(int)(dy*(y.y[i]-axes.boundary(2)+ spec.axes.axis_offset(1)))+ shape_y;
									if (debug_range_checking) {
										if (ry < 0 || ry >= (int)h)
											throw ns_ex("ns_graph::Could not draw point due to ry");
										if (rx < 0 || rx >= (int)(3 * w))
											throw ns_ex("ns_graph::Could not draw point due to rx");
									}
									image[ry][rx] = (ns_8_bit)( (1.0 - y.properties.point.opacity)*image[ry][rx] + y.properties.point.opacity*(*color)[c]);
								}
							}
						}
					}
					break;
				}
				case ns_graph_color_set::ns_circle: {
					int point_half_width = y.properties.point.width / 2;
					int edge_width = y.properties.point.edge_width;
					for (unsigned int i = 0; i < y.y.size(); i++) {
						if (y.y[i] < 0 && !y.properties.draw_negatives)
							continue;
						for (int shape_y = -point_half_width - edge_width; shape_y <= point_half_width + edge_width; shape_y++) {
							for (int shape_x = -point_half_width - edge_width; shape_x <= point_half_width + edge_width; shape_x++) {
								const ns_color_8* color(&y.properties.point.edge_color);
								bool in_circle(shape_y* shape_y + shape_x * shape_x <= (point_half_width + edge_width) * (point_half_width + edge_width));
								if (!in_circle)
									continue;
								bool edge(shape_y* shape_y + shape_x * shape_x >= (point_half_width ) * (point_half_width ));
									if (!edge)
										color = &y.properties.point.color;
								for (unsigned int c = 0; c < 3; c++) {
									int rx = 3 * (border_bl.x + (int)(dx * (x.x[i] - axes.boundary(0) + spec.axes.axis_offset(0)) + shape_x)) + c,
										ry = (int)(h - border_bl.y) - (int)(dy * (y.y[i] - axes.boundary(2) + spec.axes.axis_offset(1))) + shape_y;
									if (debug_range_checking) {
										if (ry < 0 || ry >= (int)h)
											throw ns_ex("ns_graph::Could not draw point due to ry");
										if (rx < 0 || rx >= (int)(3 * w))
											throw ns_ex("ns_graph::Could not draw point due to rx");
									}
									image[ry][rx] = (ns_8_bit)((1.0 - y.properties.point.opacity) * image[ry][rx] + y.properties.point.opacity * (*color)[c]);
								}
							}
						}
					}
					break;
				}
				case ns_graph_color_set::ns_vertical_line:{
					int point_half_width= y.properties.point.width/2;
					int line_width= y.properties.point.edge_width;
					for (unsigned int i = 0; i < y.y.size(); i++){
						if (y.y[i] < 0 && !y.properties.draw_negatives)
							continue;
						for (int shape_y = -point_half_width; shape_y <= point_half_width; shape_y++){
							for (int shape_x = -point_half_width -line_width; shape_x <= point_half_width + line_width; shape_x++){
								const ns_color_8 * color(&y.properties.point.edge_color);
								for (unsigned int c = 0; c < 3; c++){
									int rx = 3*(border_bl.x + (int)(dx*(x.x[i] - axes.boundary(0)+ spec.axes.axis_offset(0)) + shape_x))+c,
										ry = (int)(h- border_bl.y)-(int)(dy*(y.y[i]-axes.boundary(2)+ spec.axes.axis_offset(1)))+ shape_y;
									if (debug_range_checking) {
										if (ry < 0 || ry >= (int)h)
											throw ns_ex("ns_graph::Could not draw vertical line due to ry");
										if (rx < 0 || rx >= (int)(3 * w))
											throw ns_ex("ns_graph::Could not draw vertical line due to rx");
									}
									image[ry][rx] = (ns_8_bit)( (1.0 - y.properties.point.opacity)*image[ry][rx] + y.properties.point.opacity*(*color)[c]);
								}
							}
						}
					}
					break;
				}
				default:throw ns_ex("Unknown graph point type");
			}
			
		}
	}
	else if (y.type == ns_graph_object::ns_graph_vertical_line){
		for (unsigned int _y = border_tr.y; _y < h- border_bl.y- spec.dy*spec.axes.axis_offset(1); _y++){
			for (unsigned int x = 0; x < y.x.size(); x++){
				int rx = 3*(border_bl.x + (unsigned int)(dx*(y.x[x]-axes.boundary(0)+ spec.axes.axis_offset(0))));
				for (unsigned int c = 0; c < 3; c++){
					if (rx +c> 3*w) 
						break;
					if (debug_range_checking && (h - _y - 1 >= h || rx + c >= 3*w))
						throw ns_ex("Graph vertical line error");
					image[h-_y-1][rx+c] = (ns_8_bit)((1.0 - y.properties.line.opacity)*image[h-_y-1][rx+c]+ y.properties.line.opacity*y.properties.line.color[c]);
				}
			}
		}			
	}
	else if (y.type == ns_graph_object::ns_graph_horizontal_line) {
		for (unsigned int i = 0; i < y.y.size(); i++) {
			for (unsigned int pixel_x = border_bl.x; pixel_x < w - border_tr.x; pixel_x++) {
				int pixel_y = (int)(h - border_bl.y-1) - (int)(dy*(y.y[i] - axes.boundary(2) + spec.axes.axis_offset(1)));
				for (unsigned int c = 0; c < 3; c++) {
					if (debug_range_checking && (pixel_y >= h || pixel_x >= w))
						throw ns_ex("Graph vertical line error");
					image[pixel_y][3 * pixel_x + c] = (ns_8_bit)((1.0 - y.properties.line.opacity)*image[pixel_y][3 * pixel_x + c] + y.properties.line.opacity*y.properties.line.color[c]);
				}
			}
		}
	}
	else
		throw ns_ex("ns_graph::Support for specified graph object not yet supported");
	
}

void ns_graph::plot_object(const ns_graph_object & y, const ns_graph_object & x, ns_svg & svg, const ns_graph_specifics & spec){
	const ns_graph_axes & axes(spec.axes);
	const ns_vector_2i& border_bl(spec.boundary_bottom_and_left), & border_tr(spec.boundary_top_and_right);
	const double &dx(spec.dx);
	const double &dy(spec.dy);

	for (unsigned int i = 0; i < y.y.size(); i++)
		if (y.y[i] < axes.boundary(2) && y.y[i] > axes.boundary(3))
			throw ns_ex("Data point y=") << y.y[i] << " is out of specified axes bounds [" << axes.boundary(2) << "," << axes.boundary(3) << "]";

	for (unsigned int i = 0; i < x.x.size(); i++)
		if (x.x[i] < axes.boundary(0) && x.x[i] > axes.boundary(1))
			throw ns_ex("Data point x=") << x.x[i] << " is out of specified axes bounds [" << axes.boundary(0) << "," << axes.boundary(1) << "]";

	if (y.y.size() == 0)
		return;
	const unsigned int h((unsigned int)(100)),
					   w((unsigned int)(100*spec.aspect_ratio));
	
	if (y.type == ns_graph_object::ns_graph_none || y.type == ns_graph_object::ns_graph_independant_variable)
		throw ns_ex("ns_graph: Attempting to graph invalid graph object");

	else if (y.type == ns_graph_object::ns_graph_dependant_variable){
		//fill area under the curve
		if (y.properties.area_fill.draw && y.y.size() != 0){
			std::vector<ns_vector_2d> points;
			switch(y.properties.line_hold_order){
				case ns_graph_properties::ns_first:{
					for (unsigned int i = 0; i < x.x.size(); i++){
						if ((y.y[i] < 0) && !y.properties.draw_negatives)
							continue;
						const double _y(h-border_bl.y-dy*(y.y[i]-axes.boundary(2)+ spec.axes.axis_offset(1))),
									_x(border_bl.x +dx*(x.x[i] - axes.boundary(0)+ spec.axes.axis_offset(0)));
						points.push_back(ns_vector_2d(_x,_y));
					}
					break;
				}
				case ns_graph_properties::ns_zeroth:
				case ns_graph_properties::ns_zeroth_centered:{
					for (unsigned int i = 0; i < x.x.size()-1; i++){
						if ((y.y[i] < 0) && !y.properties.draw_negatives)
							continue;
						const double y0(h- border_bl.y-dy*(y.y[i]-axes.boundary(2))),
									 x0(border_bl.x +dx*(x.x[i] - axes.boundary(0)));
						const double y1(h- border_bl.y-dy*(y.y[i+1]-axes.boundary(2))),
									x1(border_bl.x +dx*(x.x[i+1] - axes.boundary(0)));
						points.push_back(ns_vector_2d(x0,y0));
						points.push_back(ns_vector_2d(x1,y0));
						points.push_back(ns_vector_2d(x1,y1));
					}
					break;
				}
			 }
			//connect to the bottom right of the graph
			if (points.size() != 0){
				points.push_back(ns_vector_2d(points[points.size()-1].x,spec.x_axis_pos));
				points.push_back(ns_vector_2d(points[0].x,spec.x_axis_pos));
				//connect to the botton left of the graph
				points.push_back(points[0]);
				svg.draw_polygon(points,ns_color_8(),y.properties.area_fill.color,y.properties.area_fill.opacity,false,true);
			}
		}
		//draw lines
		if (y.properties.line.draw){
			std::vector<ns_vector_2d> points;
			switch(y.properties.line_hold_order){
				case ns_graph_properties::ns_first:{
					for (int i = 0; i < (int)y.y.size()-1; i++){
						if ((y.y[i] < 0 || y.y[i+1] < 0) && !y.properties.draw_negatives)
							continue;
						double	x0 = border_bl.x + (dx*(x.x[i]-axes.boundary(0))),
								x1 = border_bl.x + (dx*(x.x[i+1]-axes.boundary(0))),
								y0 = h- border_bl.y - (dy*(y.y[i]-axes.boundary(2))),
								y1 = h- border_bl.y - (dy*(y.y[i+1]-axes.boundary(2)));
						points.push_back(ns_vector_2d(x0,y0));
						points.push_back(ns_vector_2d(x1,y1));
					}
					break;
				}
				case ns_graph_properties::ns_zeroth:
				case ns_graph_properties::ns_zeroth_centered:{
					for (int i = 0; i < (int)y.y.size()-1; i++){
						if ((y.y[i] < 0) && !y.properties.draw_negatives)
						continue;
						double	x0 = border_bl.x + (dx*(x.x[i]-axes.boundary(0))),
								x1 = border_bl.x + (dx*(x.x[i+1]-axes.boundary(0))),
								y0 = h- border_bl.y - (dy*(y.y[i]-axes.boundary(2)));
						points.push_back(ns_vector_2d(x0,y0));
						points.push_back(ns_vector_2d(x1,y0));
					}
					break;
				}
			}
			svg.draw_poly_line(points,y.properties.line.color,(float)(y.properties.line.width/10.0));
			if (y.properties.draw_vertical_lines){
				for (unsigned int i = 0; i < points.size(); i+=2)
					svg.draw_line(points[i],ns_vector_2d(points[i].x,spec.x_axis_pos),y.properties.line.color,(float)(y.properties.line.width/10.0));
				if (points.size() != 0)
					svg.draw_line(points[points.size()-1],ns_vector_2d(points[points.size()-1].x,spec.x_axis_pos),y.properties.line.color,(float)(y.properties.line.width/10.0));
			}
		}
		
		//draw point
		if (y.properties.point.draw){
			double point_half_width= ((double)y.properties.point.width)/20.0;
			for (unsigned int i = 0; i < y.y.size(); i++){
				if ((y.y[i] < 0) && !y.properties.draw_negatives)
					continue;
				ns_vector_2d center(border_bl.x + dx*(x.x[i] - axes.boundary(0)),h - (border_bl.y+dy*(y.y[i]-axes.boundary(2))));
				ns_vector_2d offset(point_half_width,point_half_width);
				if (y.hyperlinks.size() <= i)
					svg.draw_rectangle(center-offset,center+offset,y.properties.point.edge_color,y.properties.point.color,y.properties.point.opacity,(y.properties.point.edge_width > 0),true,y.properties.point.edge_width);
				else{
					svg.open_hyperlink(y.hyperlinks[i]);
					svg.draw_rectangle(center-offset,center+offset,y.properties.point.edge_color,y.properties.point.color,y.properties.point.opacity,(y.properties.point.edge_width > 0),true,y.properties.point.edge_width);
					svg.close_hyperlink();
				}
			}
		}
	}
	else if (y.type == ns_graph_object::ns_graph_vertical_line){
		unsigned int x = (border_bl.x + (unsigned int)(dx*(y.x[0]-axes.boundary(2))));
		svg.draw_line(ns_vector_2d(x, border_tr.y),ns_vector_2d(x,h- border_bl.y),y.properties.line.color,(float).3);
	}
	else
		throw ns_ex("ns_graph::Support for specified graph object not yet supported");
	
}


ns_graph_axes ns_graph::add_frequency_distribution(const ns_graph_object & in,const bool crop_outliers){
	std::vector<const ns_graph_object *> g(1,&in);
	return add_frequency_distribution(g);
}
double ns_find_outlier_cutoff(const std::vector<const ns_graph_object *> & in,const double y_min,unsigned long number_of_classes,double dx, const double cutoff=DBL_MAX){
	if (in.size() == 0)
		throw ns_ex("ns_find_outlier_cutoff()::Empty object std::vector provided!");
	std::vector<std::vector<unsigned long> > counts(in.size(),std::vector<unsigned long>(number_of_classes+1,0));

	for (unsigned int i = 0; i < in.size(); i++){
		for (unsigned int j = 0; j < in[i]->y.size(); j++){
			if (in[i]->y[j] > cutoff) continue;
			counts[i][(unsigned long)((in[i]->y[j]-y_min)/dx)]++;
		}
	}

	unsigned long max_significant_column_id = 0;
	for (unsigned int i = 0; i < counts.size(); i++){
		unsigned long max_sig_id=0;
		for (long j = (long)counts[i].size()-1; j >= 0; j--){
			if (counts[i][j] > in[i]->y.size()/100){
				max_sig_id = j+1; 
				break;
			}
		}
		if (max_sig_id > max_significant_column_id)
			max_significant_column_id = max_sig_id;			
	}
	//if (max_significant_column_id == 0)
	//	max_significant_column_id = (unsigned long)dx*(number_of_classes+1);
	return y_min + dx*(max_significant_column_id+1);
}

void ns_get_min_max_dx(const std::vector<const ns_graph_object *> & in, double & y_min, double & y_max, unsigned long & number_of_classes,double & dx,const double outlier_cutoff=DBL_MAX){
	y_min = DBL_MAX;
	y_max = 0;
	unsigned long y_count=0;

	//find min and max values present
	for (unsigned int j = 0; j < in.size(); j++){
		for (unsigned int i = 0; i < in[j]->y.size(); i++){
			if (in[j]->y[i] > outlier_cutoff) continue;
			if (in[j]->y[i] < y_min)
				y_min = in[j]->y[i];
			if (in[j]->y[i] > y_max)
				y_max = in[j]->y[i];
			y_count++;
		}
	}
	if (y_count == 0)
		throw ns_ex("ns_graph::add_frequency_distribution()::No data");

	if (y_count < 4)
		number_of_classes = 3;
	else if (y_count < 10)
		number_of_classes = 5;
	else if (y_count < 50)
		number_of_classes = 10;
	else if (y_count < 60)
		number_of_classes = 20;
	else if (y_count < 100)
		number_of_classes = 30;
	else number_of_classes = 40;
	if (y_max < y_min)
		throw ns_ex("ns_graph::add_frequency_distribution()::Invalid data");
	dx = (y_max - y_min)/(1.0*number_of_classes);
}

ns_graph_axes ns_graph::add_frequency_distribution(const std::vector<const ns_graph_object *> & in,const std::vector<double> & normalization, const bool crop_outliers){
	if (in.size() == 0)
		throw ns_ex("ns_graph::add_frequency_distribution()::No graph objects were provided");
	double y_min,
		   y_max,
		   dx;
	unsigned long number_of_classes;
	std::vector<double> norm = normalization;
	if (norm.size() == 0)
		norm.resize(in.size(),1.0);

	ns_get_min_max_dx(in,y_min,y_max,number_of_classes,dx);
	double outlier_cutoff = -1;
	//if we want to crop outliers, we'll have to create a histogram, find outliers, and generate a rule to exclude them.
	if (crop_outliers){
		//find the rule to exclude outliers
		outlier_cutoff = ns_find_outlier_cutoff(in,y_min,number_of_classes,dx);
		//go through and recalculate categories
		ns_get_min_max_dx(in,y_min,y_max,number_of_classes,dx,outlier_cutoff);

		//go through it again just in case there was a huge outlier.
		outlier_cutoff = ns_find_outlier_cutoff(in,y_min,number_of_classes,dx,outlier_cutoff);
		ns_get_min_max_dx(in,y_min,y_max,number_of_classes,dx,outlier_cutoff);
	}
	

	unsigned int s = (unsigned int)contents.size();
	
	stored_contents.push_back(ns_graph_object(ns_graph_object::ns_graph_independant_variable));
	
	contents.resize(s+1,&(*stored_contents.rbegin()));
	ns_graph_object & x_axis = *contents[s];
	x_axis.x.resize(number_of_classes+1);

	for (unsigned int i = 0 ; i < x_axis.x.size(); i++)
		x_axis.x[i] = dx*i + y_min;
	double cat_max(0);
	for (unsigned int j = 0; j < in.size(); j++){
		s = (unsigned int)contents.size();
		stored_contents.push_back(ns_graph_object(ns_graph_object::ns_graph_dependant_variable));
		contents.resize(s+1, &(*stored_contents.rbegin()));
		ns_graph_object & cur = *contents[s];
		cur.properties = in[j]->properties;
		cur.y.resize(number_of_classes+1);
		if (in[j]->y.size() == 0)
			continue;
		for (unsigned int i = 0; i < in[j]->y.size(); i++){
			if (crop_outliers && in[j]->y[i] >= outlier_cutoff) continue;
			int cl = (unsigned int)floor((in[j]->y[i]-y_min)/dx); // if cl < 0 is an error condition, cl must be signed
			if (cl < 0 || cl > cur.y.size())
				throw ns_ex("ns_graph::Could not plot data on frequency distribution: ") << in[j]->y[i];
			cur.y[cl]++;
		}
		for (unsigned int i = 0; i < cur.y.size(); i++)
			cur.y[i]/=(in[j]->y.size()/norm[j]);

		for (unsigned int i = 0; i < cur.y.size(); i++)
			if (cur.y[i] > cat_max)
				cat_max = cur.y[i];
	}

	
	
	ns_graph_axes axes;
	axes.axis_offset(0) = dx;
	//axes.boundary(0) = 0;
	//axes.boundary(1) = 1;
	if (number_of_classes >= 20){
		axes.tick(0) = dx*2;
	}
	else axes.tick(0) = dx;
	axes.tick(1) = dx;
	axes.tick(2) = .05;
	axes.tick(3) = .025;
	return axes;
}

