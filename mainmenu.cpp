#include "stdafx.h"

void TogglePanelSwitcher();

static const GUID guid_panel_switcher_menu_group = {0xbde6969b, 0x8b64, 0x4b50, {0x96, 0x61, 0x86, 0x4f, 0xd6, 0x28, 0x6d, 0x8a}};

static const GUID guid_panel_switcher_toggle_command = {0x52a400e8, 0xe283, 0x431a, {0xa7, 0x18, 0x5e, 0xfb, 0x79, 0x28, 0x5e, 0x4c}};

static mainmenu_group_popup_factory g_panel_switcher_menu_group(guid_panel_switcher_menu_group, mainmenu_groups::view, mainmenu_commands::sort_priority_base, "Panel Switcher");

class PanelSwitcherMenuCommands:

    public mainmenu_commands {

public:

    t_uint32 get_command_count() override {
        return 1;
    }

    GUID get_command(t_uint32 index) override {

        if (index == 0) {
            return guid_panel_switcher_toggle_command;
        }

        uBugCheck();

    }

    void get_name(t_uint32 index, pfc::string_base& out) override {

        if (index == 0) {
            out = "Toggle Panel";
            return;
        }

        uBugCheck();

    }

    bool get_description(t_uint32 index, pfc::string_base& out) override {

        if (index == 0) {
            out = "Switches between the two embedded UI elements.";
            return true;
        }

        return false;

    }

    GUID get_parent() override {
        return guid_panel_switcher_menu_group;
    }

    void execute(t_uint32 index, service_ptr_t<service_base> callback) override {

        if (index == 0) {
            TogglePanelSwitcher();
            return;
        }

        uBugCheck();

    }

};

static mainmenu_commands_factory_t<PanelSwitcherMenuCommands>
g_panel_switcher_menu_commands_factory;