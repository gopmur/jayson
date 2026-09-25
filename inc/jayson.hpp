#pragma once

#include <string>
#include <variant>
#include <concepts>
#include "cJSON.h"

template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

enum class JsonError {
  NOT_PROVIDED,
  NOT_AN_OBJECT,
  NOT_AN_ARRAY,
  NOT_A_NUMBER,
  NOT_A_STRING,
  PARSE_ERROR,
};

class JsonObject;

class Json {
  private:
  int* ref_count;
  bool owned;

  protected:
  cJSON* root;
  Json(cJSON* root, bool owned);
  std::string str;
  bool str_invalid = true;
  void release();
  
  public:
  Json& operator=(const Json& other);
  Json(const Json& other);
  ~Json();
  std::string stringify();
  static const char* get_json_error_string(JsonError error);
};

class JsonArray : public Json {
  friend JsonObject;

  private:
  JsonArray(cJSON* root, bool owned);

  public:
  JsonArray();
  static std::variant<JsonArray, JsonError> parse(char* str);

  int len();

  void append_object(JsonObject* object);
};

class JsonObject : public Json {
  friend JsonArray;

  private:
  JsonObject(cJSON* root, bool owned);

  public:
  JsonObject();
  static std::variant<JsonObject, JsonError> parse(char* str);

  bool is_empty();

  template <Numeric T>
  void set(const char* name, T number);
  void set(const char* name, JsonError error);
  void set(const char* name, const char* string);
  void set(const char* name, JsonObject* object);
  void set(const char* name, bool boolean);
  void set(const char* name, JsonArray* array);
  void add_object(const char* name);

  std::variant<JsonObject, JsonError> get_object(const char* name);
  std::variant<JsonObject, JsonError> get_object(const char* name, JsonObject* error_object);
  std::variant<double, JsonError> get_number(const char* name);
  std::variant<double, JsonError> get_number(const char* name, JsonObject* error_object);
  std::variant<char*, JsonError> get_string(const char* name);
  std::variant<char*, JsonError> get_string(const char* name, JsonObject* error_object);
};

template <Numeric T>
void JsonObject::set(const char* name, T number) {
  str_invalid = true;
  auto item = cJSON_GetObjectItem(this->root, name);
  if (item && cJSON_IsNumber(item)) {
    cJSON_SetNumberValue(item, number);
  } else if (item) {
    cJSON_ReplaceItemInObject(this->root, name, cJSON_CreateNumber(number));
  } else {
    cJSON_AddNumberToObject(this->root, name, number);
  }
}