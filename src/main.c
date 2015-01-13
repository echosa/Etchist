#include <pebble.h>
  
typedef enum {UP, DOWN, LEFT, RIGHT} direction;
direction current_direction = RIGHT;

Window* main_window;
Layer* main_layer;
TextLayer* text_layer;
ActionBarLayer* action_bar;
GRect screen_bounds;

GPoint line_start;
GPoint line_end;

int prev_points_count;
GPoint* prev_points;

bool draw_horizontally;

int min_x() {
  return screen_bounds.origin.x;
}

int max_x() {
  return min_x() + screen_bounds.size.w;
}

int min_y() {
  return screen_bounds.origin.y;  
}

int max_y() {
  return min_y() + screen_bounds.size.h;
}

void reset_screen(GPoint point) {
  line_start = point;
  prev_points_count = 0;
  prev_points = NULL;
}

void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  free(prev_points);
  reset_screen(line_end);
  layer_mark_dirty(main_layer);
}

void main_layer_update_callback(Layer* layer, GContext* context) {
  for (int count = 0; count < prev_points_count - 1; count++) {
    graphics_draw_line(context, prev_points[count], prev_points[count + 1]);
  }
  if (prev_points_count > 0) {
    graphics_draw_line(context, prev_points[prev_points_count - 1], line_start);    
  }
  graphics_draw_line(context, line_start, line_end);
  
}

void store_line() {
  GPoint* tmp_points = prev_points == NULL ? (int*)malloc(sizeof(GPoint)) : realloc(prev_points, ((prev_points_count + 1) * sizeof(GPoint)));
  if (tmp_points != NULL) {
    prev_points_count++;
    prev_points = tmp_points;
    prev_points[prev_points_count - 1] = line_start;
    line_start = line_end;
  }
}

void move_right() {
  if (line_end.x < max_x()) {
    if (current_direction == LEFT) {
      store_line();
    }
    line_end.x++;
    current_direction = RIGHT;
  }
}

void move_left() {
  if (line_end.x > min_x()) {
    if (current_direction == RIGHT) {
      store_line();
    }
    line_end.x--;
    current_direction = LEFT;
  }  
}

void move_up() {
  if (line_end.y > min_y()) {
    if (current_direction == DOWN) {
      store_line();
    }
    line_end.y--;
    current_direction = UP;
  }  
}

void move_down() {
  if (line_end.y < max_y()) {
    if (current_direction == UP) {
      store_line();
    }
    line_end.y++;
    current_direction = DOWN;
  }
}

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  draw_horizontally ? move_left() : move_up();
  layer_mark_dirty(main_layer);
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  draw_horizontally ? move_right() : move_down();
  layer_mark_dirty(main_layer);
}

void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  draw_horizontally = !draw_horizontally;
  char* text_layer_msg = draw_horizontally ? "Left/Right" : "Up/Down";
  text_layer_set_text(text_layer, text_layer_msg);
  layer_mark_dirty((Layer*) text_layer);
  store_line();
}

void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);

  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

void window_load(Window *window) {

}

void window_unload(Window *window) {

}
  
void init() {
  line_end = GPoint(20, 54);
  reset_screen(line_end);
  draw_horizontally = true;
  main_window = window_create();
  
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_set_click_config_provider(main_window, click_config_provider);
  
  window_stack_push(main_window, true);
  
  Layer* window_layer = window_get_root_layer(main_window);
  screen_bounds = layer_get_bounds(window_layer);
  screen_bounds.size.h = screen_bounds.size.h - 18;
  
  GRect main_layer_bounds = layer_get_bounds(window_layer);
  main_layer_bounds.size.h = main_layer_bounds.size.h - 16;
  
  main_layer = layer_create(main_layer_bounds);
  layer_set_update_proc(main_layer, main_layer_update_callback);
  layer_add_child(window_layer, main_layer);
  
  GRect text_layer_bounds = layer_get_bounds(window_layer);
  text_layer_bounds.origin.y = text_layer_bounds.size.h - 16;
  text_layer = text_layer_create(text_layer_bounds);
  text_layer_set_text(text_layer, "Left/Right");
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_background_color(text_layer, GColorBlack);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, (Layer*) text_layer);
  
  accel_tap_service_subscribe(accel_tap_handler);
}

void deinit() {
  window_destroy(main_window);
  layer_destroy(main_layer);
  text_layer_destroy(text_layer);
  free(prev_points);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}