/*
 * ct_main_win_multi_node_events.cc
 *
 * Copyright 2009-2026
 * Giuseppe Penone <giuspen@gmail.com>
 * Evgenii Gurianov <https://github.com/txe>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include "ct_main_win.h"
#include "ct_actions.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
CtTreeIter CtMainWin::tree_cursor_iter()
{
    if (not _uCtTreeview or not _uCtTreestore) return CtTreeIter{};
    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn* pColumn{nullptr};
    _uCtTreeview->get_cursor(path, pColumn);
    if (path.empty()) return CtTreeIter{};
    if (auto iter = _uCtTreeview->get_model()->get_iter(path)) {
        return _uCtTreestore->to_ct_tree_iter(iter);
    }
    return CtTreeIter{};
}
std::vector<CtTreeIter> CtMainWin::selected_tree_iters(bool unique_data_holders)
{
    std::vector<CtTreeIter> selected;
    if (not _uCtTreeview or not _uCtTreestore) return selected;
    std::unordered_set<gint64> seen;
    for (auto path : _uCtTreeview->get_selection()->get_selected_rows()) {
        CtTreeIter iter = _uCtTreestore->get_iter(path);
        if (iter and (not unique_data_holders or seen.insert(iter.get_node_id_data_holder()).second)) {
            selected.push_back(iter);
        }
    }
    return selected;
}
void CtMainWin::_store_previous_editor_state(CtTreeIter next_tree_iter)
{
    if (not _prevTreeIter or (_prevTreeIter.get_node_id() == next_tree_iter.get_node_id())) return;
    const gint64 prev_node_id_data_holder = _prevTreeIter.get_node_id_data_holder();
    auto pTextBuffer = _prevTreeIter.get_node_text_buffer();
    if (pTextBuffer->get_modified()) {
        _fileSaveNeeded = true;
        pTextBuffer->set_modified(false);
        _ctStateMachine.update_state(_prevTreeIter);
    }
    _nodesCursorPos[prev_node_id_data_holder] = pTextBuffer->property_cursor_position();
    if (not _multiNodeMode) {
        _nodesVScrollPos[prev_node_id_data_holder] = round(_scrolledwindowText.get_vadjustment()->get_value());
    }
}
void CtMainWin::_set_active_editor(CtTreeIter tree_iter, CtTextView* pTextView, const bool update_history)
{
    if (_multiNodeEditorRebuilding or not tree_iter or not pTextView) return;
    _store_previous_editor_state(tree_iter);
    _activeTreeIter = tree_iter;
    _pActiveTextview = pTextView;
    _prevTreeIter = tree_iter;
    if (user_active()) {
        const bool is_bookmarked = _uCtTreestore->is_node_bookmarked(tree_iter.get_node_id());
        menu_update_bookmark_menu_item(is_bookmarked);
        window_header_update();
        window_header_update_lock_icon(tree_iter.get_node_read_only());
        window_header_update_ghost_icon(tree_iter.get_node_is_excluded_from_search() or tree_iter.get_node_children_are_excluded_from_search());
        window_header_update_bookmark_icon(is_bookmarked);
        update_selected_node_statusbar_info();
    }
    if (update_history) {
        _ctStateMachine.node_selected_changed(tree_iter.get_node_id_data_holder());
    }
}