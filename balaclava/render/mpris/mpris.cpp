#include "mpris.h"

#include <cstring>
#include <dbus/dbus.h>
#include <vector>

static std::string get_string_from_variant(DBusMessageIter *variant_iter) {
  if (dbus_message_iter_get_arg_type(variant_iter) == DBUS_TYPE_STRING) {
    const char *val = nullptr;
    dbus_message_iter_get_basic(variant_iter, &val);
    return val ? val : "";
  }
  return "";
}

static std::string get_first_string_from_array(DBusMessageIter *variant_iter) {
  if (dbus_message_iter_get_arg_type(variant_iter) != DBUS_TYPE_ARRAY)
    return "";

  DBusMessageIter array_iter;
  dbus_message_iter_recurse(variant_iter, &array_iter);

  if (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING) {
    const char *val = nullptr;
    dbus_message_iter_get_basic(&array_iter, &val);
    return val ? val : "";
  }
  return "";
}

static int64_t get_int64_from_variant(DBusMessageIter *variant_iter) {
  int type = dbus_message_iter_get_arg_type(variant_iter);
  if (type == DBUS_TYPE_INT64) {
    int64_t val = 0;
    dbus_message_iter_get_basic(variant_iter, &val);
    return val;
  }
  if (type == DBUS_TYPE_UINT64) {
    uint64_t val = 0;
    dbus_message_iter_get_basic(variant_iter, &val);
    return static_cast<int64_t>(val);
  }
  return -1;
}

// Find an active MPRIS player (prefer Playing, accept Paused).
// Returns bus name and sets status_out.
static std::string find_active_player(DBusConnection *conn,
                                      std::string *status_out = nullptr) {
  DBusError err;
  dbus_error_init(&err);

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
      "ListNames");

  DBusMessage *reply =
      dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return "";
  }

  std::vector<std::string> players;
  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
    DBusMessageIter arr;
    dbus_message_iter_recurse(&iter, &arr);
    while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
      const char *name = nullptr;
      dbus_message_iter_get_basic(&arr, &name);
      if (name && std::strncmp(name, "org.mpris.MediaPlayer2.", 23) == 0)
        players.emplace_back(name);
      dbus_message_iter_next(&arr);
    }
  }
  dbus_message_unref(reply);

  std::string paused_bus;
  for (const auto &p : players) {
    msg =
        dbus_message_new_method_call(p.c_str(), "/org/mpris/MediaPlayer2",
                                     "org.freedesktop.DBus.Properties", "Get");
    const char *pi = "org.mpris.MediaPlayer2.Player";
    const char *ps = "PlaybackStatus";
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &pi, DBUS_TYPE_STRING, &ps,
                             DBUS_TYPE_INVALID);
    DBusMessage *status_reply =
        dbus_connection_send_with_reply_and_block(conn, msg, 200, &err);
    dbus_message_unref(msg);
    if (status_reply && !dbus_error_is_set(&err)) {
      DBusMessageIter si;
      dbus_message_iter_init(status_reply, &si);
      if (dbus_message_iter_get_arg_type(&si) == DBUS_TYPE_VARIANT) {
        DBusMessageIter sv;
        dbus_message_iter_recurse(&si, &sv);
        if (dbus_message_iter_get_arg_type(&sv) == DBUS_TYPE_STRING) {
          const char *status = nullptr;
          dbus_message_iter_get_basic(&sv, &status);
          if (status && std::strcmp(status, "Playing") == 0) {
            if (status_out) *status_out = status;
            dbus_message_unref(status_reply);
            return p;
          }
          if (status && std::strcmp(status, "Paused") == 0 &&
              paused_bus.empty()) {
            paused_bus = p;
            if (status_out) *status_out = status;
          }
        }
      }
      dbus_message_unref(status_reply);
    } else {
      dbus_error_free(&err);
    }
  }

  return paused_bus;
}

// Get a property as int64 from the Player interface.
static int64_t get_player_int64_property(DBusConnection *conn,
                                         const char *bus,
                                         const char *prop_name) {
  DBusError err;
  dbus_error_init(&err);

  DBusMessage *msg = dbus_message_new_method_call(
      bus, "/org/mpris/MediaPlayer2",
      "org.freedesktop.DBus.Properties", "Get");
  const char *iface = "org.mpris.MediaPlayer2.Player";
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING,
                           &prop_name, DBUS_TYPE_INVALID);

  DBusMessage *reply =
      dbus_connection_send_with_reply_and_block(conn, msg, 200, &err);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return -1;
  }

  int64_t result = -1;
  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
    DBusMessageIter v;
    dbus_message_iter_recurse(&iter, &v);
    result = get_int64_from_variant(&v);
  }
  dbus_message_unref(reply);
  return result;
}

// Call a no-arg method on the active player.
static void call_player_method(const char *method) {
  DBusError err;
  dbus_error_init(&err);

  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (!conn || dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return;
  }

  std::string bus = find_active_player(conn);
  if (bus.empty()) return;

  DBusMessage *msg = dbus_message_new_method_call(
      bus.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", method);
  dbus_connection_send(conn, msg, nullptr);
  dbus_connection_flush(conn);
  dbus_message_unref(msg);
}

NowPlaying mpris_now_playing() {
  NowPlaying result;

  DBusError err;
  dbus_error_init(&err);

  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (!conn || dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return result;
  }

  std::string player_bus = find_active_player(conn, &result.playback_status);
  if (player_bus.empty())
    return result;

  // Get Metadata property
  DBusMessage *msg = dbus_message_new_method_call(
      player_bus.c_str(), "/org/mpris/MediaPlayer2",
      "org.freedesktop.DBus.Properties", "Get");

  const char *iface = "org.mpris.MediaPlayer2.Player";
  const char *prop = "Metadata";
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING,
                           &prop, DBUS_TYPE_INVALID);

  DBusMessage *reply =
      dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return result;
  }

  // Parse variant -> a{sv}
  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    dbus_message_unref(reply);
    return result;
  }

  DBusMessageIter variant;
  dbus_message_iter_recurse(&iter, &variant);

  if (dbus_message_iter_get_arg_type(&variant) != DBUS_TYPE_ARRAY) {
    dbus_message_unref(reply);
    return result;
  }

  DBusMessageIter dict;
  dbus_message_iter_recurse(&variant, &dict);

  while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry;
    dbus_message_iter_recurse(&dict, &entry);

    const char *key = nullptr;
    dbus_message_iter_get_basic(&entry, &key);
    dbus_message_iter_next(&entry);

    DBusMessageIter val_variant;
    dbus_message_iter_recurse(&entry, &val_variant);

    if (key && std::strcmp(key, "xesam:title") == 0) {
      result.title = get_string_from_variant(&val_variant);
    } else if (key && std::strcmp(key, "xesam:artist") == 0) {
      result.artist = get_first_string_from_array(&val_variant);
    } else if (key && std::strcmp(key, "mpris:length") == 0) {
      result.length_us = get_int64_from_variant(&val_variant);
    }

    dbus_message_iter_next(&dict);
  }

  dbus_message_unref(reply);

  // Get Position property (separate from Metadata)
  result.position_us =
      get_player_int64_property(conn, player_bus.c_str(), "Position");

  return result;
}

void mpris_play_pause() { call_player_method("PlayPause"); }
void mpris_next() { call_player_method("Next"); }
void mpris_previous() { call_player_method("Previous"); }

static void adjust_volume(double delta) {
  DBusError err;
  dbus_error_init(&err);

  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (!conn || dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return;
  }

  std::string bus = find_active_player(conn);
  if (bus.empty()) return;

  // Get current volume
  double vol = 1.0;
  {
    DBusMessage *msg = dbus_message_new_method_call(
        bus.c_str(), "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Get");
    const char *iface = "org.mpris.MediaPlayer2.Player";
    const char *prop = "Volume";
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING,
                             &prop, DBUS_TYPE_INVALID);
    DBusMessage *reply =
        dbus_connection_send_with_reply_and_block(conn, msg, 200, &err);
    dbus_message_unref(msg);
    if (reply && !dbus_error_is_set(&err)) {
      DBusMessageIter iter;
      dbus_message_iter_init(reply, &iter);
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
        DBusMessageIter v;
        dbus_message_iter_recurse(&iter, &v);
        if (dbus_message_iter_get_arg_type(&v) == DBUS_TYPE_DOUBLE)
          dbus_message_iter_get_basic(&v, &vol);
      }
      dbus_message_unref(reply);
    } else {
      dbus_error_free(&err);
      return;
    }
  }

  // Set new volume
  vol += delta;
  if (vol < 0.0) vol = 0.0;
  if (vol > 1.0) vol = 1.0;

  {
    DBusMessage *msg = dbus_message_new_method_call(
        bus.c_str(), "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Set");
    const char *iface = "org.mpris.MediaPlayer2.Player";
    const char *prop = "Volume";
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING,
                             &prop, DBUS_TYPE_INVALID);
    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);
    DBusMessageIter variant;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "d", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_DOUBLE, &vol);
    dbus_message_iter_close_container(&iter, &variant);
    dbus_connection_send(conn, msg, nullptr);
    dbus_connection_flush(conn);
    dbus_message_unref(msg);
  }
}

void mpris_volume_up() { adjust_volume(0.05); }
void mpris_volume_down() { adjust_volume(-0.05); }
