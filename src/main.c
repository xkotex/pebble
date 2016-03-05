#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static GFont s_time_font;
static GFont s_weather_font;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Хранит входящую информацию
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Читает кортежи для данных
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

  // Если все данные доступны, то использует это
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);

    // Собирает полную строку и отображает ее
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void update_time() {
  // Получаем структура времени
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Пишем время в данный момент в буфер
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Дисплей на текстовом слое
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Получает обновление температуры каждые 30 минут
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void main_window_load(Window *window) {
  // Получаем информацию о окне
Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Создаем GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);

  // Создаем BitmapLayer на дисплей the GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Устанавливаем bitmap в слой и добавляем окно
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  // Создаем определенный текстовый слой с границами
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  
  // Улучшаем расположение циферблата
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Создаем GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));

  // Применяем TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  
  //Добавляем дочерний слой
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Создаем температурный слой
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));
  
  // Стиль текста
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "loading...");

  // Создаем второй шрифт, и применяем его
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
  text_layer_set_font(s_weather_layer, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
}
  
static void main_window_unload(Window *window) {
  // Убираем текстовый слой
  text_layer_destroy(s_time_layer);
  
  // Выгружаем GFont
  fonts_unload_custom_font(s_time_font);
  
  // Уничтожаем GBitmap
  gbitmap_destroy(s_background_bitmap);

  // Уничтожаем BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Уничтожаем погодные элементы
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
}


static void init() {
  // Создаем главный элемент окна и присваиваем ему значение
  s_main_window = window_create();
  
  // Устанавливаем цвет фона
  window_set_background_color(s_main_window, GColorBlack);

  // Устанавливаем обработчик окна
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Показываем окно на часах, с анимацией
  window_stack_push(s_main_window, true);
  
  // Время отображается с самого начала
  update_time();

  // Регистрация в ТикТаймСервисе
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Регистрация вызово
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Открытие сообщений
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Уничтожаем окно
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
