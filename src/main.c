#include <pebble.h>

#if PBL_IF_ROUND_ELSE(1, 0) == 1
  #define PBL_ROUND 1
#else
  #define PBL_RECT 1
#endif

#ifdef DBG
#define log(...) APP_LOG(APP_LOG_LEVEL_DEBUG, __VA_ARGS__);
#else
#define log(...) ((void) 0)
#endif

typedef struct {
  GPoint origin;
  uint8_t radi;
} circle_t;

typedef enum {UP, DOWN, LEFT, RIGHT} direction;
direction current_direction = RIGHT;

Window* main_window;
Layer* main_layer;
TextLayer* text_layer;
ActionBarLayer* action_bar;

#ifdef PBL_ROUND
  static circle_t screen_bounds;
#else
  static GRect screen_bounds;
#endif

static GRect label_bounds = {
  .size.h = PBL_IF_ROUND_ELSE(22,18)
};

static GPoint line_start;
static GPoint line_end;

static int prev_points_count;
static GPoint* prev_points;

static bool draw_horizontally;

static bool circle_contains_pt(circle_t *circle, GPoint *pt) {
  int16_t dx = circle->origin.x - pt->x;
  int16_t dy = circle->origin.y - pt->y;
  return dx * dx + dy * dy <= circle->radi * circle->radi;
}

static bool canvas_contains_pt(GPoint *pt) {
  bool contains = !grect_contains_point(&label_bounds, pt)
               && PBL_IF_ROUND_ELSE(circle_contains_pt, grect_contains_point)(&screen_bounds, pt);
  log("x=%d y=%d ret=%s", pt->x, pt->y, contains ? "true" : "false");
  return contains;
}

static void reset_screen(GPoint point) {
  line_start = point;
  prev_points_count = 0;
  prev_points = NULL;
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  free(prev_points);
  reset_screen(line_end);
  layer_mark_dirty(main_layer);
}

static void main_layer_update_callback(Layer* layer, GContext* context) {
  for (int count = 0; count < prev_points_count - 1; count++) {
    graphics_draw_line(context, prev_points[count], prev_points[count + 1]);
  }
  if (prev_points_count > 0) {
    graphics_draw_line(context, prev_points[prev_points_count - 1], line_start);    
  }
  graphics_draw_line(context, line_start, line_end);
}

static void store_line() {
  GPoint* tmp_points = prev_points == NULL ? (int*)malloc(sizeof(GPoint)) : realloc(prev_points, ((prev_points_count + 1) * sizeof(GPoint)));
  if (tmp_points != NULL) {
    prev_points_count++;
    prev_points = tmp_points;
    prev_points[prev_points_count - 1] = line_start;
    line_start = line_end;
  }
}

static void move_right() {
  line_end.x++;
  if (canvas_contains_pt(&line_end)) {
    if (current_direction == LEFT) {
      store_line();
    }
    current_direction = RIGHT;
  } else {
    line_end.x--;
  }
}

static void move_left() {
  line_end.x--;
  if (canvas_contains_pt(&line_end)) {
    if (current_direction == RIGHT) {
      store_line();
    }
    current_direction = LEFT;
  } else {
    line_end.x++;
  }
}

static void move_up() {
  line_end.y--;
  if (canvas_contains_pt(&line_end)) {
    if (current_direction == DOWN) {
      store_line();
    }
    current_direction = UP;
  } else {
    line_end.y++;
  }
}

static void move_down() {
  line_end.y++;
  if (canvas_contains_pt(&line_end)) {
    if (current_direction == UP) {
      store_line();
    }
    current_direction = DOWN;
  } else {
    line_end.y--;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  draw_horizontally ? move_left() : move_up();
  layer_mark_dirty(main_layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  draw_horizontally ? move_right() : move_down();
  layer_mark_dirty(main_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  draw_horizontally = !draw_horizontally;
  char* text_layer_msg = draw_horizontally ? "Left/Right" : "Up/Down";
  text_layer_set_text(text_layer, text_layer_msg);
  layer_mark_dirty((Layer*) text_layer);
  store_line();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);

  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

static void window_load(Window *window) {

}

static void window_unload(Window *window) {

}

static void init_screen_bounds(GRect *bounds) {
  screen_bounds.origin = bounds->origin;

  #ifdef PBL_ROUND
  screen_bounds.origin.x += bounds->size.w / 2;
  screen_bounds.origin.y += bounds->size.h / 2;
  screen_bounds.radi = bounds->size.w / 2;
  #else
  screen_bounds.size = bounds->size;
  #endif

  #ifdef PBL_RECT
  log("w=%d h=%d", screen_bounds.size.w, screen_bounds.size.h);
  #else
  log("radi=%d", screen_bounds.radi);
  #endif
}

static void init_label_bounds(GRect *screen_bounds) {
  label_bounds.origin = screen_bounds->origin;
  label_bounds.origin.y += screen_bounds->size.h - label_bounds.size.h;
  label_bounds.size.w = screen_bounds->size.w;
}

static void init() {
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
  GRect screen_rect = layer_get_bounds(window_layer);
  init_screen_bounds(&screen_rect);

  init_label_bounds(&screen_rect);

  main_layer = layer_create(screen_rect);
  layer_set_update_proc(main_layer, main_layer_update_callback);
  layer_add_child(window_layer, main_layer);

  text_layer = text_layer_create(label_bounds);
  text_layer_set_text(text_layer, "Left/Right");
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_background_color(text_layer, GColorBlack);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, (Layer*) text_layer);
  
  accel_tap_service_subscribe(accel_tap_handler);
}

static void deinit() {
  window_destroy(main_window);
  layer_destroy(main_layer);
  text_layer_destroy(text_layer);
  free(prev_points);
}

int main() {
  init();
  app_event_loop();
  deinit();
}