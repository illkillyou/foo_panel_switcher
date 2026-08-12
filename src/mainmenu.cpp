#include "stdafx.h"

void PanelSwitcherNext();
void PanelSwitcherPrevious();
void PanelSwitcherAdd();
void PanelSwitcherRemoveCurrent();

void PanelSwitcherSelect(t_size index);
void PanelSwitcherGetChildName(t_size index,pfc::string_base& out);

t_size PanelSwitcherGetCount();
t_size PanelSwitcherGetActiveIndex();

static const GUID guid_panel_switcher_menu_group = {0xbde6969b, 0x8b64, 0x4b50, {0x96, 0x61, 0x86, 0x4f, 0xd6, 0x28, 0x6d, 0x8a}};
static const GUID guid_panel_switcher_panels_group = {0x9d85b0db, 0xa31c, 0x4981, {0x86, 0x6d, 0xa4, 0xf9, 0x77, 0x34, 0x0e, 0x31}};

static const GUID guid_panel_switcher_previous_command = {0xd295e070, 0xb9c2, 0x43d9, { 0x95, 0xd1, 0x27, 0xc8, 0xf4, 0x37, 0x65, 0xe1}};
static const GUID guid_panel_switcher_next_command = {0xf899b6e4, 0x68e2, 0x485c, {0xbb, 0xb4, 0xb1, 0x0e, 0x04, 0x42, 0xfc, 0xb6}};
static const GUID guid_panel_switcher_add_command = {0x574436b4, 0x9056, 0x4e77, {0xba, 0xef, 0x4e, 0x92, 0xa0, 0xe7, 0x7f, 0x21}};
static const GUID guid_panel_switcher_remove_command = {0xef036859, 0xb928, 0x4ab8, {0xb7, 0x5e, 0x0e, 0x57, 0x61, 0xf9, 0x1a, 0xcf}};

static mainmenu_group_popup_factory g_panel_switcher_menu_group(guid_panel_switcher_menu_group, mainmenu_groups::view, mainmenu_commands::sort_priority_base, "Panel Switcher");
static mainmenu_group_popup_factory g_panel_switcher_panels_group(guid_panel_switcher_panels_group, guid_panel_switcher_menu_group, mainmenu_commands::sort_priority_last, "Panels");

struct MenuCommand {

    GUID guid;
    const char* name;
    const char* description;
    void (*execute)();

};

static const MenuCommand commands[] = {
    
    {guid_panel_switcher_previous_command, "Previous Panel", "Switches to the previous panel.", PanelSwitcherPrevious},
    {guid_panel_switcher_next_command, "Next Panel", "Switches to the next panel.", PanelSwitcherNext},
    {guid_panel_switcher_add_command, "Add Panel", "Adds another panel slot.", PanelSwitcherAdd},
    {guid_panel_switcher_remove_command, "Remove Current Panel", "Removes the currently active panel.", PanelSwitcherRemoveCurrent}

};

class PanelSwitcherMenuCommands:

    public mainmenu_commands {

public:

    t_uint32 get_command_count() override {
        return (t_uint32)std::size(commands);
    }

    GUID get_command(t_uint32 index) override {
        return commands[index].guid;
    }

    void get_name(t_uint32 index, pfc::string_base& out) override {
        out = commands[index].name;
    }

    bool get_description(t_uint32 index, pfc::string_base& out) override {
        out = commands[index].description;
        return true;
    }

    GUID get_parent() override {
        return guid_panel_switcher_menu_group;
    }

    void execute(t_uint32 index, service_ptr_t<service_base> callback) override {
        commands[index].execute();
    }

};

static mainmenu_commands_factory_t<PanelSwitcherMenuCommands>
g_panel_switcher_menu_commands_factory;

static GUID make_panel_command_guid(t_uint32 index) {

    GUID guid = { 0x83100000, 0x8427, 0x4c31, {0xa8, 0xa4, 0x17, 0x73, 0xd2, 0x6c, 0x51, 0x90} };

    guid.Data1 += index;
    return guid;
}

class PanelSwitcherPanelCommands :

    public mainmenu_commands {

public:

    t_uint32 get_command_count() override {
        return (t_uint32)PanelSwitcherGetCount();
    }

    GUID get_command(t_uint32 index) override {

        if (index < PanelSwitcherGetCount()) {
            return make_panel_command_guid(index);
        }

        uBugCheck();

    }

    void get_name(t_uint32 index, pfc::string_base& out) override {

        if (index >= PanelSwitcherGetCount()) {
            uBugCheck();
        }

        pfc::string_formatter name;

        if (index == PanelSwitcherGetActiveIndex()) {
            name << "[Active] ";
        }
        
        pfc::string8 panelName;
        PanelSwitcherGetChildName(index, panelName);
        name << panelName;

        out = name;

    }

    bool get_description(t_uint32 index, pfc::string_base& out) override {

        if (index >= PanelSwitcherGetCount()) {
            return false;
        }

        out = "Switches directly to this panel.";

        return true;

    }

    GUID get_parent() override {
        return guid_panel_switcher_panels_group;
    }


    void execute(t_uint32 index, service_ptr_t<service_base> callback) override {

        if (index >= PanelSwitcherGetCount()) {
            return;
        }

        PanelSwitcherSelect(index);

    }

};

static mainmenu_commands_factory_t<PanelSwitcherPanelCommands>
g_panel_switcher_panel_commands_factory;