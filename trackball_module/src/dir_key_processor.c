#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <drivers/input_processor.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/virtual_key_position.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_input_processor_dir_keys

#define DIR_KEY_BINDING(idx, inst) ZMK_KEYMAP_EXTRACT_BINDING(idx, inst)
#define DIR_KEY_BINDING_COUNT 4

enum dir_key_binding_index {
    DIR_KEY_X_POS = 0,
    DIR_KEY_X_NEG,
    DIR_KEY_Y_POS,
    DIR_KEY_Y_NEG,
};

BUILD_ASSERT(DIR_KEY_BINDING_COUNT == (DIR_KEY_Y_NEG + 1),
             "DIR_KEY_BINDING_COUNT must match dir key binding enum length");

struct dir_key_state {
    int32_t accumulator;
};

struct dir_key_config {
    uint8_t index;
    int16_t x_code;
    int16_t y_code;
    uint16_t threshold;
    const struct zmk_behavior_binding bindings[DIR_KEY_BINDING_COUNT];
};

struct dir_key_data {
    struct dir_key_state x;
    struct dir_key_state y;
};

static int dir_key_trigger_binding(const struct zmk_behavior_binding *binding,
                                   struct zmk_input_processor_state *state, uint8_t index) {
    struct zmk_behavior_binding_event behavior_event = {
        .position = ZMK_VIRTUAL_KEY_POSITION_BEHAVIOR_INPUT_PROCESSOR(state->input_device_index,
                                                                      index),
        .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    int err = zmk_behavior_invoke_binding(binding, behavior_event, true);
    if (err < 0) {
        LOG_ERR("Failed to press binding %s (%d)", binding->behavior_dev, err);
        return err;
    }

    err = zmk_behavior_invoke_binding(binding, behavior_event, false);
    if (err < 0) {
        LOG_ERR("Failed to release binding %s (%d)", binding->behavior_dev, err);
        return err;
    }

    return 0;
}

static int dir_key_handle_axis(struct dir_key_state *state, int32_t delta, uint16_t threshold,
                               const struct zmk_behavior_binding *positive,
                               const struct zmk_behavior_binding *negative,
                               struct zmk_input_processor_state *proc_state, uint8_t index) {
    int err = 0;

    state->accumulator += delta;

    while (state->accumulator >= threshold) {
        state->accumulator -= threshold;
        err = dir_key_trigger_binding(positive, proc_state, index);
        if (err) {
            return err;
        }
    }

    while (state->accumulator <= -threshold) {
        state->accumulator += threshold;
        err = dir_key_trigger_binding(negative, proc_state, index);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int dir_key_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                                uint32_t param2, struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    const struct dir_key_config *cfg = dev->config;
    struct dir_key_data *data = dev->data;

    if (event->type != INPUT_EV_REL || cfg->threshold == 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int err = 0;
    if (event->code == cfg->x_code) {
        err = dir_key_handle_axis(&data->x, event->value, cfg->threshold,
                                  &cfg->bindings[DIR_KEY_X_POS], &cfg->bindings[DIR_KEY_X_NEG],
                                  state, cfg->index);
    } else if (event->code == cfg->y_code) {
        err = dir_key_handle_axis(&data->y, event->value, cfg->threshold,
                                  &cfg->bindings[DIR_KEY_Y_POS], &cfg->bindings[DIR_KEY_Y_NEG],
                                  state, cfg->index);
    } else {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (err) {
        return err;
    }

    return ZMK_INPUT_PROC_STOP;
}

static const struct zmk_input_processor_driver_api dir_key_driver_api = {
    .handle_event = dir_key_handle_event,
};

static int dir_key_init(const struct device *dev) {
    struct dir_key_data *data = dev->data;
    data->x.accumulator = 0;
    data->y.accumulator = 0;
    return 0;
}

#define DIR_KEY_INST(n)                                                                            \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, bindings) == DIR_KEY_BINDING_COUNT,                           \
                 "bindings must contain four entries (x+, x-, y+, y-)");                           \
    static const struct zmk_behavior_binding dir_key_bindings_##n[] = {                            \
        LISTIFY(DIR_KEY_BINDING_COUNT, DIR_KEY_BINDING, (, ), DT_DRV_INST(n))};                    \
    static struct dir_key_data dir_key_data_##n;                                                   \
    static const struct dir_key_config dir_key_config_##n = {                                      \
        .index = n,                                                                                \
        .x_code = DT_INST_PROP_OR(n, x_code, INPUT_REL_X),                                         \
        .y_code = DT_INST_PROP_OR(n, y_code, INPUT_REL_Y),                                         \
        .threshold = DT_INST_PROP_OR(n, threshold, 8),                                             \
        .bindings = {                                                                              \
            dir_key_bindings_##n[DIR_KEY_X_POS],                                                   \
            dir_key_bindings_##n[DIR_KEY_X_NEG],                                                   \
            dir_key_bindings_##n[DIR_KEY_Y_POS],                                                   \
            dir_key_bindings_##n[DIR_KEY_Y_NEG],                                                   \
        },                                                                                         \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, dir_key_init, NULL, &dir_key_data_##n, &dir_key_config_##n,           \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &dir_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DIR_KEY_INST)
