#include "mpris.h"

#include <dbus/dbus.h>
#include <cstring>
#include <vector>

static std::string get_string_from_variant(DBusMessageIter* variant_iter) {
    if (dbus_message_iter_get_arg_type(variant_iter) == DBUS_TYPE_STRING) {
        const char* val = nullptr;
        dbus_message_iter_get_basic(variant_iter, &val);
        return val ? val : "";
    }
    return "";
}

static std::string get_first_string_from_array(DBusMessageIter* variant_iter) {
    if (dbus_message_iter_get_arg_type(variant_iter) != DBUS_TYPE_ARRAY)
        return "";

    DBusMessageIter array_iter;
    dbus_message_iter_recurse(variant_iter, &array_iter);

    if (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING) {
        const char* val = nullptr;
        dbus_message_iter_get_basic(&array_iter, &val);
        return val ? val : "";
    }
    return "";
}

NowPlaying mpris_now_playing() {
    NowPlaying result;

    DBusError err;
    dbus_error_init(&err);

    DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn || dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return result;
    }

    // List names to find an MPRIS player
    DBusMessage* msg = dbus_message_new_method_call(
        "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "ListNames");

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
    dbus_message_unref(msg);

    if (!reply || dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return result;
    }

    // Collect all MPRIS players
    std::vector<std::string> players;
    DBusMessageIter iter;
    dbus_message_iter_init(reply, &iter);
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        DBusMessageIter arr;
        dbus_message_iter_recurse(&iter, &arr);
        while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
            const char* name = nullptr;
            dbus_message_iter_get_basic(&arr, &name);
            if (name && std::strncmp(name, "org.mpris.MediaPlayer2.", 23) == 0) {
                players.emplace_back(name);
            }
            dbus_message_iter_next(&arr);
        }
    }
    dbus_message_unref(reply);

    // Find the player that is currently "Playing"
    std::string player_bus;
    for (const auto& p : players) {
        msg = dbus_message_new_method_call(
            p.c_str(), "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties", "Get");
        const char* pi = "org.mpris.MediaPlayer2.Player";
        const char* ps = "PlaybackStatus";
        dbus_message_append_args(msg,
            DBUS_TYPE_STRING, &pi,
            DBUS_TYPE_STRING, &ps,
            DBUS_TYPE_INVALID);
        DBusMessage* status_reply = dbus_connection_send_with_reply_and_block(conn, msg, 200, &err);
        dbus_message_unref(msg);
        if (status_reply && !dbus_error_is_set(&err)) {
            DBusMessageIter si;
            dbus_message_iter_init(status_reply, &si);
            if (dbus_message_iter_get_arg_type(&si) == DBUS_TYPE_VARIANT) {
                DBusMessageIter sv;
                dbus_message_iter_recurse(&si, &sv);
                if (dbus_message_iter_get_arg_type(&sv) == DBUS_TYPE_STRING) {
                    const char* status = nullptr;
                    dbus_message_iter_get_basic(&sv, &status);
                    if (status && std::strcmp(status, "Playing") == 0) {
                        player_bus = p;
                        dbus_message_unref(status_reply);
                        break;
                    }
                }
            }
            dbus_message_unref(status_reply);
        } else {
            dbus_error_free(&err);
        }
    }

    if (player_bus.empty())
        return result;

    // Get Metadata property
    msg = dbus_message_new_method_call(
        player_bus.c_str(), "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Get");

    const char* iface = "org.mpris.MediaPlayer2.Player";
    const char* prop = "Metadata";
    dbus_message_append_args(msg,
        DBUS_TYPE_STRING, &iface,
        DBUS_TYPE_STRING, &prop,
        DBUS_TYPE_INVALID);

    reply = dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
    dbus_message_unref(msg);

    if (!reply || dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return result;
    }

    // Parse variant -> a{sv}
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

        const char* key = nullptr;
        dbus_message_iter_get_basic(&entry, &key);
        dbus_message_iter_next(&entry);

        DBusMessageIter val_variant;
        dbus_message_iter_recurse(&entry, &val_variant);

        if (key && std::strcmp(key, "xesam:title") == 0) {
            result.title = get_string_from_variant(&val_variant);
        } else if (key && std::strcmp(key, "xesam:artist") == 0) {
            result.artist = get_first_string_from_array(&val_variant);
        }

        dbus_message_iter_next(&dict);
    }

    dbus_message_unref(reply);
    return result;
}
