#include "stdafx.h"

#include <libPPUI/win32_op.h>
#include <helpers/BumpableElem.h>
#include <helpers/ui_element_helpers.h>

namespace {

    class PanelSwitcherWindow;
    static pfc::list_t<PanelSwitcherWindow*> g_panelSwitchers;
    static PanelSwitcherWindow* g_lastActivePanelSwitcher = nullptr;

    static const GUID guid_panel_switcher = {0xf784188a, 0x6408, 0x47cf, {0x85, 0x8c, 0x35, 0xaf, 0x8e, 0xe8, 0xb9, 0x57}};

    static ui_element_config::ptr makeConfig(t_size activeChild, ui_element_config::ptr child1, ui_element_config::ptr child2) {

        ui_element_config_builder out;

        t_uint32 version = 1;
        t_uint32 active = (t_uint32)activeChild;

        out << version;
        out << active;
        out << child1;
        out << child2;

        return out.finish(guid_panel_switcher);

    }

    static void parseConfig(ui_element_config::ptr config, t_size& activeChild, ui_element_config::ptr& child1, ui_element_config::ptr& child2) {
        
        try {

            ui_element_config_parser in(config);

            t_uint32 version;
            t_uint32 active;

            in >> version;

            if (version != 1) {
                throw exception_io_data();
            }

            in >> active;
            in >> child1;
            in >> child2;

            activeChild = active == 1 ? 1 : 0;

        }

        catch (exception_io_data) {

            activeChild = 0;

            child1 = ui_element_config::g_create_empty();
            child2 = ui_element_config::g_create_empty();

        }

    }

    class PanelSwitcherWindow:

        public ui_element_helpers::ui_element_instance_host_base,
        public CWindowImpl<PanelSwitcherWindow> {

    public:

        ~PanelSwitcherWindow();
        void ToggleChild();
        
        DECLARE_WND_CLASS_EX(TEXT("{5A31E7E3-1A39-455B-95CB-0F5794270A38}"), CS_VREDRAW | CS_HREDRAW, (-1));

        void initialize_window(HWND parent) {

            WIN32_OP(Create(parent) != NULL);

            if (m_childConfig1.is_empty() || m_childConfig1->get_guid() == pfc::guid_null) {

                m_child1 = ui_element_helpers::instantiate_dummy(*this,ui_element_config::g_create_empty(), ui_element_instance_callback_get_ptr(0));

            }else{

                m_child1 = ui_element_helpers::instantiate(*this,m_childConfig1, ui_element_instance_callback_get_ptr(0));

            }

            if (m_childConfig2.is_empty() || m_childConfig2->get_guid() == pfc::guid_null) {
               
                m_child2 = ui_element_helpers::instantiate_dummy(*this,ui_element_config::g_create_empty(), ui_element_instance_callback_get_ptr(1));

            }else{
               
                m_child2 = ui_element_helpers::instantiate(*this, m_childConfig2, ui_element_instance_callback_get_ptr(1));
            }

            CRect rc;
            GetClientRect(&rc);

            if (m_child1.is_valid()) {
                ::SetWindowPos(m_child1->get_wnd(), NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
            }

            if (m_child2.is_valid()) {
                ::SetWindowPos(m_child2->get_wnd(), NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
            }

            if (m_child1.is_valid()) {
                ::ShowWindow(m_child1->get_wnd(), m_activeChild == 0 ? SW_SHOW : SW_HIDE);
            }

            if (m_child2.is_valid()) {
                ::ShowWindow(m_child2->get_wnd(), m_activeChild == 1 ? SW_SHOW : SW_HIDE);
            }

        }

        BEGIN_MSG_MAP_EX(PanelSwitcherWindow)

            MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown);
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_SIZE(OnSize)
            CHAIN_MSG_MAP(ui_element_helpers::ui_element_instance_host_base)

        END_MSG_MAP()

        PanelSwitcherWindow(ui_element_config::ptr, ui_element_instance_callback_ptr p_callback);

        HWND get_wnd() {
            return *this;
        }

        void set_configuration(ui_element_config::ptr config) {

            parseConfig(config, m_activeChild, m_childConfig1, m_childConfig2);

            if (m_hWnd != NULL) {
                
                m_child1 = ui_element_helpers::update(m_child1, *this, m_childConfig1, ui_element_instance_callback_get_ptr(0));
                m_child2 = ui_element_helpers::update(m_child2, *this, m_childConfig2, ui_element_instance_callback_get_ptr(1));

                CRect rc;
                GetClientRect(&rc);

                ::SetWindowPos(m_child1->get_wnd(), NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
                ::SetWindowPos(m_child2->get_wnd(), NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);

                ::ShowWindow(m_child1->get_wnd(), m_activeChild == 0 ? SW_SHOW : SW_HIDE);
                ::ShowWindow(m_child2->get_wnd(), m_activeChild == 1 ? SW_SHOW : SW_HIDE);

            }
        }

        ui_element_config::ptr get_configuration() {

            ui_element_config::ptr child1Config = m_childConfig1;
            ui_element_config::ptr child2Config = m_childConfig2;

            if (m_child1.is_valid()) {
                child1Config = m_child1->get_configuration();
            }

            if (m_child2.is_valid()) {
                child2Config = m_child2->get_configuration();
            }

            return makeConfig(m_activeChild, child1Config, child2Config);

        }

        static GUID g_get_guid() {
            return guid_panel_switcher;
        }

        static GUID g_get_subclass() {
            return ui_element_subclass_containers;
        }

        static void g_get_name(pfc::string_base& out) {
            out = "Panel Switcher";
        }

        static ui_element_config::ptr g_get_default_configuration() {
            return makeConfig(0, ui_element_config::g_create_empty(), ui_element_config::g_create_empty());
        }

        static const char* g_get_description() {
            return "Container for switching between foobar2000 UI elements.";
        }

        void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

    private:

        LRESULT OnLButtonDown(UINT, WPARAM, LPARAM, BOOL&) {

            g_lastActivePanelSwitcher = this;
            m_callback->request_replace(this);
            return 0;

        }

        void OnPaint(CDCHandle);
        void OnSize(UINT, CSize);
        BOOL OnEraseBkgnd(CDCHandle);

        ui_element_config::ptr m_childConfig1;
        ui_element_config::ptr m_childConfig2;

        ui_element_instance_ptr m_child1;
        ui_element_instance_ptr m_child2;

        t_size m_activeChild = 0;

        ui_element_instance_ptr host_get_child(t_size which) override {

            if (which == 0) {
                return m_child1;
            }

            if (which == 1) {
                return m_child2;
            }

            return nullptr;

        }

        t_size host_get_children_count() override {

            if (!m_child1.is_valid() && !m_child2.is_valid()) {
                return 0;
            }

            return 2;

        }

        bool host_is_child_visible(t_size which) override {
            return which == m_activeChild;
        }

        void host_replace_child(t_size which) override {

            if (which >= 2) {
                return;
            }

            ui_element_instance_ptr child = host_get_child(which);

            GUID currentGuid = pfc::guid_null;

            if (child.is_valid()) {
                currentGuid = child->get_guid();
            }

            replace_dialog(*this, (unsigned)which, currentGuid);

        }

        void host_replace_element(unsigned p_id, const GUID& p_newguid) override {

            if (p_id >= 2) {
                return;
            }

            ui_element_instance_ptr* child;

            if (p_id == 0) {
                child = &m_child1;
            }else{
                child = &m_child2;
            }

            ui_element_helpers::replace_with_new_element(*child, p_newguid, *this, ui_element_instance_callback_get_ptr(p_id));

            if (p_id == 0) {
                m_childConfig1 = m_child1->get_configuration();
            }else{
                m_childConfig2 = m_child2->get_configuration();
            }

            CRect rc;
            GetClientRect(&rc);

            ::SetWindowPos((*child)->get_wnd(), NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
            ::ShowWindow((*child)->get_wnd(), p_id == m_activeChild ? SW_SHOW : SW_HIDE);

        }

    };

    void PanelSwitcherWindow::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) {

        if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed) {
            Invalidate();
        }

        ui_element_helpers::ui_element_instance_host_base::notify(p_what,p_param1, p_param2, p_param2size);

    }

    void PanelSwitcherWindow::OnSize(UINT, CSize size) {

        if (m_child1.is_valid()) {
            ::SetWindowPos(m_child1->get_wnd(), NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (m_child2.is_valid()) {
            ::SetWindowPos(m_child2->get_wnd(), NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
        }

    }

    void PanelSwitcherWindow::ToggleChild() {

        g_lastActivePanelSwitcher = this;

        if (!m_child1.is_valid() || !m_child2.is_valid()) {
            return;
        }

        t_size oldChild = m_activeChild;

        m_activeChild = (m_activeChild == 0) ? 1 : 0;

        ::ShowWindow(m_child1->get_wnd(), m_activeChild == 0 ? SW_SHOW : SW_HIDE);

        ::ShowWindow(m_child2->get_wnd(), m_activeChild == 1 ? SW_SHOW : SW_HIDE);

        host_child_visibility_changed(oldChild, false);
        host_child_visibility_changed(m_activeChild, true);

        ::SetWindowPos(host_get_child(m_activeChild)->get_wnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    }

    PanelSwitcherWindow::PanelSwitcherWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) : ui_element_instance_host_base(p_callback) {

        parseConfig(config, m_activeChild, m_childConfig1, m_childConfig2);

        g_panelSwitchers.add_item(this);
        g_lastActivePanelSwitcher = this;

    }

    PanelSwitcherWindow::~PanelSwitcherWindow() {

        g_panelSwitchers.remove_item(this);

        if (g_lastActivePanelSwitcher == this) {

            if (g_panelSwitchers.get_count() > 0) {
                g_lastActivePanelSwitcher = g_panelSwitchers[g_panelSwitchers.get_count() - 1];
            }else{
                g_lastActivePanelSwitcher = nullptr;
            }

        }

    }

    BOOL PanelSwitcherWindow::OnEraseBkgnd(CDCHandle dc) {

        CRect rc;
        WIN32_OP_D(GetClientRect(&rc));

        CBrush brush;
        WIN32_OP_D(brush.CreateSolidBrush(m_callback->query_std_color(ui_color_background)) != NULL);

        WIN32_OP_D(dc.FillRect(&rc, brush));

        return TRUE;

    }

    void PanelSwitcherWindow::OnPaint(CDCHandle) {

        CPaintDC dc(*this);

        dc.SetTextColor(m_callback->query_std_color(ui_color_text));
        dc.SetBkMode(TRANSPARENT);

        SelectObjectScope fontScope(dc, (HGDIOBJ)m_callback->query_font_ex(ui_font_default));

        const UINT format = DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE;

        CRect rc;
        WIN32_OP_D(GetClientRect(&rc));

        WIN32_OP_D(dc.DrawText(_T("Panel Switcher"), -1, &rc, format ) > 0);

    }

    class PanelSwitcherElement: 
        public ui_element_impl_withpopup<PanelSwitcherWindow> {};

    static service_factory_single_t<PanelSwitcherElement> g_ui_element_myimpl_factory;

}

void TogglePanelSwitcher() {

    if (g_lastActivePanelSwitcher != nullptr) {
        g_lastActivePanelSwitcher->ToggleChild();
    }

}