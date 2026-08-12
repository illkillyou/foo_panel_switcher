#include "stdafx.h"

#include <libPPUI/win32_op.h>
#include <helpers/BumpableElem.h>
#include <helpers/ui_element_helpers.h>

namespace {

    class PanelSwitcherWindow;
    static pfc::list_t<PanelSwitcherWindow*> g_panelSwitchers;
    static PanelSwitcherWindow* g_lastActivePanelSwitcher = nullptr;

    static const GUID guid_panel_switcher = {0xf784188a, 0x6408, 0x47cf, {0x85, 0x8c, 0x35, 0xaf, 0x8e, 0xe8, 0xb9, 0x57}};

    static ui_element_config::ptr makeConfig(t_size activeChild, const pfc::list_t<ui_element_config::ptr>& children) {

        ui_element_config_builder out;

        t_uint32 version = 2;
        t_uint32 active = (t_uint32)activeChild;
        t_uint32 count = (t_uint32)children.get_count();

        out << version;
        out << active;
        out << count;

        for (t_size i = 0; i < children.get_count(); i++) {
            out << children[i];
        }

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

    static void parseConfig(ui_element_config::ptr config, t_size& activeChild, pfc::list_t<ui_element_config::ptr>& children) {

        try {

            ui_element_config_parser in(config);

            t_uint32 version;
            in >> version;

            children.remove_all();

            if (version == 2) {

                t_uint32 active;
                t_uint32 count;

                in >> active;
                in >> count;

                for (t_uint32 i = 0; i < count; i++) {

                    ui_element_config::ptr child;
                    in >> child;
                    children.add_item(child);

                }

                if (children.get_count() == 0) {
                    children.add_item(ui_element_config::g_create_empty());
                }

                activeChild = active < children.get_count() ? active : 0;

                return;

            }

            throw exception_io_data();

        }

        catch (exception_io_data) {

            activeChild = 0;

            children.remove_all();
            children.add_item(ui_element_config::g_create_empty());
            children.add_item(ui_element_config::g_create_empty());

        }

    }

    class PanelSwitcherWindow:

        public ui_element_helpers::ui_element_instance_host_base,
        public CWindowImpl<PanelSwitcherWindow> {

    public:

        ~PanelSwitcherWindow();

        void AddChild();
        void NextChild();
        void PreviousChild();
        void RemoveActiveChild();
        void SetActiveChild(t_size index);

        void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

        void GetChildName(t_size index, pfc::string_base& out) {

            if (index >= m_children.get_count() || !m_children[index].is_valid() || m_children[index]->get_guid() == pfc::guid_null) {

                pfc::string_formatter name;
                name << "Empty Panel " << (index + 1);
                out = name;
                return;

            }

            ui_element::ptr element;

            if (ui_element::g_find(element, m_children[index]->get_guid())) {

                element->get_name(out);
                return;

            }

            out = "Unknown Panel";

        }

        void initialize_window(HWND parent) {

            WIN32_OP(Create(parent) != NULL);
            RebuildChildren();

        }

        void set_configuration(ui_element_config::ptr config) {

            parseConfig(config, m_activeChild, m_childConfigs);

            if (m_hWnd != NULL) {
                RebuildChildren();
            }

        }

        t_size GetChildCount() const {
            return m_childConfigs.get_count();
        }

        t_size GetActiveChild() const {
            return m_activeChild;
        }
        
        HWND get_wnd() {
            return *this;
        }

        ui_element_config::ptr get_configuration() {

            CaptureChildConfigs();
            return makeConfig(m_activeChild, m_childConfigs);

        }

        DECLARE_WND_CLASS_EX(TEXT("{5A31E7E3-1A39-455B-95CB-0F5794270A38}"), CS_VREDRAW | CS_HREDRAW, (-1));

        BEGIN_MSG_MAP_EX(PanelSwitcherWindow)

            MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown);
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_SIZE(OnSize)
            CHAIN_MSG_MAP(ui_element_helpers::ui_element_instance_host_base)

        END_MSG_MAP()

        PanelSwitcherWindow(ui_element_config::ptr, ui_element_instance_callback_ptr p_callback);

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

            pfc::list_t<ui_element_config::ptr> children;

            children.add_item(ui_element_config::g_create_empty());
            children.add_item(ui_element_config::g_create_empty());
            return makeConfig(0, children);

        }

        static const char* g_get_description() {
            return "Container for switching between foobar2000 UI elements.";
        }

    private:

        LRESULT OnLButtonDown(UINT, WPARAM, LPARAM, BOOL&) {

            g_lastActivePanelSwitcher = this;
            m_callback->request_replace(this);
            return 0;

        }

        void OnPaint(CDCHandle);
        void OnSize(UINT, CSize);
        BOOL OnEraseBkgnd(CDCHandle);

        pfc::list_t<ui_element_config::ptr> m_childConfigs;
        pfc::list_t<ui_element_instance_ptr> m_children;

        t_size m_activeChild = 0;

        void CaptureChildConfigs() {

            t_size count = pfc::min_t(m_children.get_count(), m_childConfigs.get_count());

            for (t_size i = 0; i < count; i++) {

                if (m_children[i].is_valid()) {
                    m_childConfigs[i] = m_children[i]->get_configuration();
                }

            }

        }

        void RebuildChildren() {

            m_children.remove_all();

            if (m_childConfigs.get_count() == 0) {

                m_childConfigs.add_item(ui_element_config::g_create_empty());
                m_activeChild = 0;

            }

            if (m_activeChild >= m_childConfigs.get_count()) {
                m_activeChild = 0;
            }

            for (t_size i = 0; i < m_childConfigs.get_count(); i++) {

                ui_element_instance_ptr child;

                if (m_childConfigs[i].is_empty() || m_childConfigs[i]->get_guid() == pfc::guid_null) {
                    child = ui_element_helpers::instantiate_dummy(*this, ui_element_config::g_create_empty(), ui_element_instance_callback_get_ptr(i));
                }else{
                    child = ui_element_helpers::instantiate(*this, m_childConfigs[i], ui_element_instance_callback_get_ptr(i));
                }

                m_children.add_item(child);

            }

            CRect rc;
            GetClientRect(&rc);

            for (t_size i = 0; i < m_children.get_count(); i++) {

                if (!m_children[i].is_valid()) {
                    continue;
                }

                ::SetWindowPos(m_children[i]->get_wnd(), NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
                ::ShowWindow(m_children[i]->get_wnd(), i == m_activeChild ? SW_SHOW : SW_HIDE);

            }

        }

        ui_element_instance_ptr host_get_child(t_size which) override {

            if (which >= m_children.get_count()) {
                return nullptr;
            }

            return m_children[which];

        }

        t_size host_get_children_count() override {
            return m_children.get_count();
        }

        bool host_is_child_visible(t_size which) override {
            return which == m_activeChild;
        }

        void host_replace_child(t_size which) override {

            if (which >= m_children.get_count()) {
                return;
            }

            ui_element_instance_ptr child = m_children[which];
            GUID currentGuid = pfc::guid_null;

            if (child.is_valid()) {
                currentGuid = child->get_guid();
            }

            replace_dialog(*this, (unsigned)which, currentGuid);

        }

        void host_replace_element(unsigned p_id, const GUID& p_newguid) override {

            if (p_id >= m_children.get_count()) {
                return;
            }

            ui_element_instance_ptr* child = &m_children[p_id];

            ui_element_helpers::replace_with_new_element(*child, p_newguid, *this, ui_element_instance_callback_get_ptr(p_id));
            m_childConfigs[p_id] = (*child)->get_configuration();

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

        for (t_size i = 0; i < m_children.get_count(); i++) {

            if (!m_children[i].is_valid()) {
                continue;
            }

            ::SetWindowPos(m_children[i]->get_wnd(), NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);

        }

    }

    void PanelSwitcherWindow::SetActiveChild(t_size index) {

        if (index >= m_children.get_count()) {
            return;
        }

        g_lastActivePanelSwitcher = this;
        t_size oldChild = m_activeChild;
        m_activeChild = index;

        for (t_size i = 0; i < m_children.get_count(); i++) {

            if (!m_children[i].is_valid()) {
                continue;
            }

            ::ShowWindow(m_children[i]->get_wnd(), i == m_activeChild ? SW_SHOW : SW_HIDE);

        }

        if (oldChild != m_activeChild) {

            host_child_visibility_changed(oldChild, false);
            host_child_visibility_changed(m_activeChild,true);

        }

        ::SetWindowPos(m_children[m_activeChild]->get_wnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    }

    void PanelSwitcherWindow::NextChild() {

        t_size count = m_children.get_count();

        if (count == 0) {
            return;
        }

        SetActiveChild((m_activeChild + 1) % count);

    }

    void PanelSwitcherWindow::PreviousChild() {

        t_size count = m_children.get_count();

        if (count == 0) {
            return;
        }

        SetActiveChild(m_activeChild == 0 ? count - 1 : m_activeChild - 1);
    }

    void PanelSwitcherWindow::AddChild() {

        CaptureChildConfigs();

        m_childConfigs.add_item(ui_element_config::g_create_empty());
        m_activeChild = m_childConfigs.get_count() - 1;

        RebuildChildren();
        host_replace_child(m_activeChild);

    }

    void PanelSwitcherWindow::RemoveActiveChild() {

        if (m_childConfigs.get_count() <= 1) {
            return;
        }

        CaptureChildConfigs();
        m_childConfigs.remove_by_idx(m_activeChild);

        if (m_activeChild >= m_childConfigs.get_count()) {
            m_activeChild = m_childConfigs.get_count() - 1;
        }

        RebuildChildren();

    }

    PanelSwitcherWindow::PanelSwitcherWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) : ui_element_instance_host_base(p_callback) {

        parseConfig(config, m_activeChild, m_childConfigs);

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

void PanelSwitcherNext() {

    if (g_lastActivePanelSwitcher != nullptr) {
        g_lastActivePanelSwitcher->NextChild();
    }

}

void PanelSwitcherPrevious() {

    if (g_lastActivePanelSwitcher != nullptr) {
        g_lastActivePanelSwitcher->PreviousChild();
    }

}

void PanelSwitcherSelect(t_size index) {

    if (g_lastActivePanelSwitcher != nullptr) {
        g_lastActivePanelSwitcher->SetActiveChild(index);
    }

}

void PanelSwitcherAdd() {

    if (g_lastActivePanelSwitcher != nullptr) {
        g_lastActivePanelSwitcher->AddChild();
    }

}

void PanelSwitcherRemoveCurrent() {

    if (g_lastActivePanelSwitcher != nullptr) {
        g_lastActivePanelSwitcher->RemoveActiveChild();
    }

}

void PanelSwitcherGetChildName(t_size index, pfc::string_base& out) {

    if (g_lastActivePanelSwitcher == nullptr) {

        out = "Panel";
        return;

    }

    g_lastActivePanelSwitcher->GetChildName(index, out);

}

t_size PanelSwitcherGetCount() {

    if (g_lastActivePanelSwitcher == nullptr) {
        return 0;
    }

    return g_lastActivePanelSwitcher->GetChildCount();

}

t_size PanelSwitcherGetActiveIndex() {

    if (g_lastActivePanelSwitcher == nullptr) {
        return 0;
    }

    return g_lastActivePanelSwitcher->GetActiveChild();

}