#!/usr/bin/python3

import subprocess
import argparse
import logging
import pprint
import json
import sys
import re
import os
import gi

gi.require_version('Gtk', '3.0')
from gi.repository import GLib, Gio, Gtk, Gdk, Pango

from subprocess import Popen, PIPE

GSCOPE_ROOT = os.path.abspath(os.path.dirname(__file__))
CSCOPE_VERSION = '0.3'
CSCOPE_NAME = 'GScope'

def conf_prepend_path(prefix, path):
    return os.path.join(GSCOPE_ROOT, prefix, path)

def conf_map_paths(conf):
    for key in conf['img'].keys():
        conf['img'][key] = conf_prepend_path('data', conf['img'][key])

CSCOPE_KEY_VERBOSE          = 0
CSCOPE_KEY_CROSSREF         = 1
CSCOPE_KEY_NOCASE           = 2
CSCOPE_KEY_NOCOMP           = 3
CSCOPE_KEY_DRY              = 4
CSCOPE_KEY_KERNEL           = 5
CSCOPE_KEY_SINGLE           = 6
CSCOPE_KEY_LUI              = 7
CSCOPE_KEY_INVERT           = 8
CSCOPE_KEY_FORCE            = 9
CSCOPE_KEY_QRY_REFERENCES   = 10
CSCOPE_KEY_QRY_DEFINITION   = 11
CSCOPE_KEY_QRY_CALLED_FUNC  = 12
CSCOPE_KEY_QRY_CALLING_FUNC = 13
CSCOPE_KEY_QRY_TEXT         = 14
CSCOPE_KEY_QRY_EGREP        = 15
CSCOPE_KEY_QRY_FILE         = 16
CSCOPE_KEY_QRY_INCLUDING    = 17

cscope_query_params = {
    CSCOPE_KEY_VERBOSE:             ['-v', 'Verbose'],
    CSCOPE_KEY_CROSSREF:            ['-b', 'Build the cross-reference only'],
    CSCOPE_KEY_NOCASE:              ['-C', 'Ignore letter case'],
    CSCOPE_KEY_NOCOMP:              ['-c', 'Do not compress the data'],
    CSCOPE_KEY_DRY:                 ['-d', 'Do not update the cross-reference'],
    CSCOPE_KEY_KERNEL:              ['-k', 'Kernel mode'],
    CSCOPE_KEY_SINGLE:              ['-L', 'Single search'],
    CSCOPE_KEY_LUI:                 ['-l', 'Line oriented interface'],
    CSCOPE_KEY_INVERT:              ['-q', 'Inverted symbols'],
    CSCOPE_KEY_FORCE:               ['-u', 'Unconditional rebuild'],
    CSCOPE_KEY_QRY_REFERENCES:      ['-0', 'Find References'],
    CSCOPE_KEY_QRY_DEFINITION:      ['-1', 'Find Definition'],
    CSCOPE_KEY_QRY_CALLED_FUNC:     ['-2', 'Find Called Functions'],
    CSCOPE_KEY_QRY_CALLING_FUNC:    ['-3', 'Find Calling Functions'],
    CSCOPE_KEY_QRY_TEXT:            ['-4', 'Find Text'],
    CSCOPE_KEY_QRY_EGREP:           ['-5', 'Find Egrep Pattern'],
    CSCOPE_KEY_QRY_FILE:            ['-7', 'Find File'],
    CSCOPE_KEY_QRY_INCLUDING:       ['-8', 'Find Including']
}

def get_cscope_param_key(t):
    return cscope_query_params[t][0]

def get_cscope_param_help(t):
    return cscope_query_params[t][1]

class PyCscope:
    def __init__(self, log, conf, sym, sym_type):
        self.results = [ ]
        self.log = log
        self.conf = conf
        self.do_query(sym, sym_type)

    def do_query(self, sym, sym_type):
        #
        # {file}<Space>{function}<Space>{line}<Space>{declaration}
        #
        output = Popen(['cscope', '-q', get_cscope_param_key(sym_type), sym,
                        get_cscope_param_key(CSCOPE_KEY_DRY),
                        get_cscope_param_key(CSCOPE_KEY_SINGLE)], stdout = PIPE)
        for line in output.communicate()[0].decode('utf-8').split('\n'):
            if len(line) == 0:
                continue
            v = line.split(' ', 3)
            if len(v) < 4:
                continue
            self.log.debug('file %s line %s text %s' % (v[0], v[2], v[3]))
            self.results.append(v)
        output.wait()

tag_codes = {
    'ctags': {
        '0': 'None',
        'c': 'Class',
        'd': 'Macro',
        'e': 'Enumerator',
        'f': 'Function',
        'g': 'Enum',
        'l': 'Local',
        'm': 'Member',
        'n': 'Namespace',
        'p': 'Prototype',
        's': 'Structure',
        't': 'Typedef',
        'u': 'Union',
        'v': 'Variable',
        'x': 'Externvar'
    },
    'gotags': {
        'p': 'Package',
        'i': 'Import',
        'c': 'Constant',
        'v': 'Variable',
        't': 'Type',
        'n': 'Interface',
        'w': 'Field',
        'e': 'Embedded',
        'm': 'Method',
        'r': 'Constructor',
        'f': 'Function'
    }
}

class PyTags:
    def __init__(self, log, conf, filename):
        self.tags = [ ]
        self.log = log
        self.conf = conf
        self.tag_type = None
        self.parse(self.tags, filename)

    def decode(self, tag):
        v = 'Unknown'
        if self.tag_type in tag_codes:
            if tag[3][0] in tag_codes[self.tag_type]:
                v = tag_codes[self.tag_type][tag[3][0]]
                return tag[0], tag[1], tag[2].split(';')[0], v

    def parse(self, tags, filename):
        self.log.debug('parsing %s' % (filename))
        cmd = self.conf['tags']['*']
        for key, val in self.conf['tags'].items():
            if key == '*':
                continue
            m = re.match(key, filename)
            if m != None:
                cmd = val
                break
        self.tag_type = cmd[0]
        cmd = cmd[1][:]
        cmd.append(filename)
        self.log.debug('cmd %s' % (repr(cmd)))
        #
        # {tagname}<Tab>{tagfile}<Tab>{tagaddress}
        #
        output = Popen(cmd, stdout = PIPE)
        for line in output.communicate()[0].decode('utf-8').split('\n'):
            if len(line) == 0 or line[0] == '!':
                continue
            v = line.split('\t')
            if len(v) < 4:
                continue
            self.log.debug('name %32s file %26s line %4s type %s' %
                           (self.decode(v)))
            tags.append(v)
        self.log.debug('%d objects parsed' % (len(tags)))
        output.wait()

def ui_InputDialogCB(widget, dialog):
    dialog.response(Gtk.ResponseType.OK)

def ui_InputDialog(parent, message = '', title = ''):
    d = Gtk.MessageDialog(parent,
                          Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                          Gtk.MessageType.QUESTION,
                          Gtk.ButtonsType.OK_CANCEL,
                          message)
    d.set_title(title)
    e = Gtk.Entry()
    e.connect('activate', ui_InputDialogCB, d)
    e.set_size_request(250, 0)
    d.get_content_area().pack_end(e, False, False, 0)
    d.show_all()
    resp = d.run()
    text = e.get_text()
    d.destroy()
    if (resp == Gtk.ResponseType.OK) and (text != ''):
        return text
    return None

UI_INFO = '''
<ui>
    <menubar name='MenuBar'>
        <menu action='FileMenu'>
            <menuitem action='FileOpen' />
            <separator />
            <menuitem action='FileExit' />
        </menu>
        <menu action='ProjectMenu'>
            <menuitem action='ProjectOpen' />
            <menuitem action='ProjectSave' />
            <menuitem action='ProjectClose' />
        </menu>
        <menu action='CScopeMenu'>
            <menuitem action='CScopeDefinition' />
            <menuitem action='CScopeReferences' />
            <menuitem action='CScopeCalledBy' />
            <menuitem action='CScopeCalling' />
        </menu>
    </menubar>
</ui>
'''

class GScope(Gtk.Window):
    def ui_gen_title(self):
        title = CSCOPE_NAME + '-' + CSCOPE_VERSION
        if self.project_file != None:
            title += ': ' + self.project_file
            if self.project_modified == True:
                title += ' *'
        return title

    def ui_on_modify(self):
        if self.project_modified == False:
            self.project_modified = True
            if self.project_file != None:
                self.props.title = self.ui_gen_title()

    def gen_entry_open(self, path):
        return {'action': 'open', 'path': path}

    def gen_entry_cscope(self, sym, sym_type):
        return {'action': 'cscope', 'sym': sym, 'type': sym_type}

    def __init__(self, conf, log):
        self.conf = conf
        self.log = log
        self.project_file = None
        self.project_modified = False
        self.project_settings = { }
        self.cwd = os.getcwd() + '/'
        self.project_settings['program'] = CSCOPE_NAME
        self.project_settings['version'] = CSCOPE_VERSION
        self.project_settings['cwd'] = self.cwd
        self.project_settings['entry'] = [ ]
        Gtk.Window.__init__(self, title = self.ui_gen_title())
        self.uiScreen = Gdk.Screen.get_default()
        self.default_width = int(self.uiScreen.get_width() * 0.75)
        self.default_height = int(self.uiScreen.get_height() * 0.75)
        self.set_default_size(self.default_width, self.default_height)

        self.set_icon_from_file(self.conf['img']['icon'])

        uiActionGroup = Gtk.ActionGroup("py_actions")
        uiActionGroup.add_action(Gtk.Action("FileMenu", "File", None, None))

        uiActionFileOpen = Gtk.Action("FileOpen", "Open", "Open a file", Gtk.STOCK_OPEN)
        uiActionFileOpen.connect("activate", self.on_uiMenuFileOpen)
        uiActionGroup.add_action_with_accel(uiActionFileOpen, None)

        uiActionFileExit = Gtk.Action("FileExit", "Exit", "Exit program", Gtk.STOCK_QUIT)
        uiActionFileExit.connect("activate", self.on_uiMenuFileExit)
        uiActionGroup.add_action_with_accel(uiActionFileExit, None)

        uiActionGroup.add_action(Gtk.Action("ProjectMenu", "Project", None, None))

        uiActionProjectOpen = Gtk.Action("ProjectOpen", "Open", None, None)
        uiActionProjectOpen.connect("activate", self.on_uiMenuProjectOpen)
        uiActionGroup.add_action_with_accel(uiActionProjectOpen, None)

        uiActionProjectSave = Gtk.Action("ProjectSave", "Save", None, None)
        uiActionProjectSave.connect("activate", self.on_uiMenuProjectSave)
        uiActionGroup.add_action_with_accel(uiActionProjectSave, None)

        uiActionProjectClose = Gtk.Action("ProjectClose", "Close", None, None)
        uiActionProjectClose.connect("activate", self.on_uiMenuProjectClose)
        uiActionGroup.add_action_with_accel(uiActionProjectClose, None)

        uiActionGroup.add_action(Gtk.Action("CScopeMenu", "CScope", None, None))

        uiActionCscopeDefinition = Gtk.Action("CScopeDefinition", "Definition", None, None)
        uiActionCscopeDefinition.connect("activate", self.on_uiMenuCscope,
                                         ("Definition", CSCOPE_KEY_QRY_DEFINITION))
        uiActionGroup.add_action_with_accel(uiActionCscopeDefinition, None)

        uiActionCscopeReferences = Gtk.Action("CScopeReferences", "References", None, None)
        uiActionCscopeReferences.connect("activate", self.on_uiMenuCscope,
                                         ("References", CSCOPE_KEY_QRY_REFERENCES))
        uiActionGroup.add_action_with_accel(uiActionCscopeReferences, None)

        uiActionCscopeCalledby = Gtk.Action("CScopeCalledBy", "Called By", None, None)
        uiActionCscopeCalledby.connect("activate", self.on_uiMenuCscope,
                                       ("Called By", CSCOPE_KEY_QRY_CALLED_FUNC))
        uiActionGroup.add_action_with_accel(uiActionCscopeCalledby, None)

        uiActionCscopeCalling = Gtk.Action("CScopeCalling", "Calling", None, None)
        uiActionCscopeCalling.connect("activate", self.on_uiMenuCscope,
                                      ("Calling", CSCOPE_KEY_QRY_CALLING_FUNC))
        uiActionGroup.add_action_with_accel(uiActionCscopeCalling, None)

        self.uiManager = Gtk.UIManager()
        self.uiManager.add_ui_from_string(UI_INFO)
        self.add_accel_group(self.uiManager.get_accel_group())
        self.uiManager.insert_action_group(uiActionGroup)

        self.uiMenubar = self.uiManager.get_widget('/MenuBar')
        self.uiTopBox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL)
        self.uiTopBox.pack_start(self.uiMenubar, False, True, 0)

        self.uiPannedTop = Gtk.Paned(orientation = Gtk.Orientation.VERTICAL)
        self.uiTopBox.pack_start(self.uiPannedTop, True, True, 0)

        self.uiNotebookSource = Gtk.Notebook()
        self.uiPannedTop.pack1(self.uiNotebookSource, True, False)

        self.notebook_source_pages = { }

        self.uiNotebookCscope = Gtk.Notebook()
        self.uiPannedTop.pack2(self.uiNotebookCscope, True, False)

        self.uiStatusbar = Gtk.Statusbar()
        self.uiStatusbarCtxID = self.uiStatusbar.get_context_id('')
        self.uiStatusbarMsgID = self.uiStatusbar.push(self.uiStatusbarCtxID, os.getcwd())
        self.uiTopBox.pack_start(self.uiStatusbar, False, True, 0)
        self.add(self.uiTopBox)

        self.uiPannedTop.set_position(int(self.default_height
                                          * float(self.conf["ui"]["source-view-ratio"])))

        self.connect('delete-event', Gtk.main_quit)
        self.show_all()

    def save_project(self, path):
        self.log.debug(path)
        with open(path, 'w') as f:
            json.dump(self.project_settings, f, indent = 4)
        self.project_modified = False
        self.project_file = path
        self.props.title = self.ui_gen_title()

    def load_project(self, path):
        self.log.debug(path)
        self.project_file = path
        with open(path, 'r') as f:
            proj = json.load(f)
            self.log.debug(pprint.pformat(proj))
            if 'cwd' in proj:
                self.cwd = proj['cwd']
                self.log.debug("change cwd to %s" % self.cwd)
                os.chdir(self.cwd)
            for key, val in proj.items():
                self.log.debug("key %s val %s" % (repr(key), repr(val)))
                if key == 'entry':
                    for entry in val:
                        self.log.debug("entry: %s" % pprint.pformat(entry))
                        if 'action' not in entry:
                            continue
                        if entry['action'] == 'open' and \
                                'path' in entry:
                            self.ui_AddNotebookSourcePage(entry['path'], None)
                        elif entry['action'] == 'cscope':
                            if 'sym' in entry and 'type' in entry:
                                self.ui_AddNotebookCscopePage(entry['sym'],
                                                              int(entry['type']))
            self.project_modified = False
            self.props.title = self.ui_gen_title()

    def get_notebook_source_page(self, page_nr):
        if page_nr in self.notebook_source_pages:
            return self.notebook_source_pages[page_nr]
        return (None, None, None, None)

    def lookup_notebook_source_page(self, path):
        for page_nr, v in self.notebook_source_pages.items():
            self.log.debug("cscope lookup: path %s v %s" % (path, v[0]))
            if v[0] == path:
                return page_nr, v
        return (None, None)

    def ui_ScrollTextView(self, uiTextView, line):
        if line == None:
            return
        uiTextViewBuffer = uiTextView.get_buffer()
        start = uiTextViewBuffer.get_iter_at_line(line - 1)
        stop = uiTextViewBuffer.get_iter_at_line(line)
        uiTextViewBuffer.select_range(start, stop)
        while Gtk.events_pending():
            Gtk.main_iteration()
        uiTextView.scroll_to_iter(start, 0.25, False, 0, 0)

    def ui_GenNotebookTitle(self, title):
        uiHBox = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL)
        uiCmdLock = Gtk.Button()
        uiCmdLock.props.relief = 2
        uiCmdLock.priv_images = [Gtk.Image().new_from_file(self.conf["img"]["locked"]),
                                 Gtk.Image().new_from_file(self.conf["img"]["unlocked"])]
        uiCmdLock.priv_locked = False
        uiCmdLock.set_image(uiCmdLock.priv_images[1])
        uiLabel = Gtk.Label(title)
        uiCmdClose = Gtk.Button()
        uiCmdClose.props.relief = 2
        uiCmdClose.set_image(Gtk.Image().new_from_file(self.conf["img"]["close"]))

        uiHBox.pack_start(uiCmdLock, False, True, 0)
        uiHBox.pack_start(uiLabel, False, True, 0)
        uiHBox.pack_start(uiCmdClose, False, True, 0)
        return (uiHBox, uiCmdLock, uiLabel, uiCmdClose)

    def on_uiTextViewSrcPopUpSelected(self, widget, data):
        query_type = data
        page_nr = self.uiNotebookSource.get_current_page()
        path, uiStoreTags, uiTreeTags, uiTextViewSrc = self.get_notebook_source_page(page_nr)
        if path != None:
            uiTextViewBuffer = uiTextViewSrc.get_buffer()
            start, end = uiTextViewBuffer.get_selection_bounds()
            text = uiTextViewBuffer.get_text(start, end, False)
            self.ui_AddNotebookCscopePage(text, query_type)

    def ui_PopupPush(self, uiMenu, uiMenuItem, callback, data):
        uiMenu.insert(uiMenuItem, 0)
        if callback != None:
            uiMenuItem.connect('activate', callback, data)
        uiMenuItem.show()

    def on_uiTextViewSrcPopUp(self, uiTextView, uiMenu):
        self.ui_PopupPush(uiMenu, Gtk.SeparatorMenuItem.new(),
                          None, None)
        self.ui_PopupPush(uiMenu, Gtk.MenuItem.new_with_label("Calling"),
                          self.on_uiTextViewSrcPopUpSelected, CSCOPE_KEY_QRY_CALLING_FUNC)
        self.ui_PopupPush(uiMenu, Gtk.MenuItem.new_with_label("Called By"),
                          self.on_uiTextViewSrcPopUpSelected, CSCOPE_KEY_QRY_CALLED_FUNC)
        self.ui_PopupPush(uiMenu, Gtk.MenuItem.new_with_label("References"),
                          self.on_uiTextViewSrcPopUpSelected, CSCOPE_KEY_QRY_REFERENCES)
        self.ui_PopupPush(uiMenu, Gtk.MenuItem.new_with_label("Definition"),
                          self.on_uiTextViewSrcPopUpSelected, CSCOPE_KEY_QRY_DEFINITION)

    def on_uiTextViewSrcKeyPress(self, widget, event):
        if event.type != Gdk.EventType.KEY_PRESS:
            return
        if (event.state & Gdk.ModifierType.CONTROL_MASK) == 0:
            return
        if event.keyval == Gdk.KEY_e:
            page_nr = self.uiNotebookSource.get_current_page()
            path, uiStoreTags, uiTreeTags, uiTextViewSrc = self.get_notebook_source_page(page_nr)
            if path != None:
                uiTextViewBuffer = uiTextViewSrc.get_buffer()
                cmd = self.conf['editor'][:]
                cmd.append(path)

                uiIter = uiTextViewBuffer.get_iter_at_mark(uiTextViewBuffer.get_insert())
                line = uiIter.get_line()
                col = uiIter.get_line_offset()

                if cmd[0] == "vim" or cmd[0] == "gvim":
                    cmd.append('+ call cursor(' + str(line + 1)
                               + ',' + str(col + 1)
                               + ')')
                self.log.debug("cmd %s" % repr(cmd))
                subprocess.call(cmd)

    def on_uiNotebookLock(self, widget, uiCmdLock):
        if uiCmdLock.priv_locked == False:
            uiCmdLock.priv_locked = True
            uiCmdLock.set_image(uiCmdLock.priv_images[0])
        else:
            uiCmdLock.priv_locked = False
            uiCmdLock.set_image(uiCmdLock.priv_images[1])

    def on_uiNotebookClose(self, widget, data):
        uiNotebookSource, uiCmdLock, page_nr, nbStore, action, adata1, adata2 = data
        if uiCmdLock.priv_locked == False:
            if nbStore != None:
                del nbStore[page_nr]
            if action == 'open':
                self.project_settings['entry'].remove(self.gen_entry_open(adata1))
            elif action == 'cscope':
                self.project_settings['entry'].remove(self.gen_entry_cscope(adata1,
                                                                            adata2))
            uiNotebookSource.remove_page(page_nr)
            uiNotebookSource.show_all()
            self.ui_on_modify()

    def on_uiSourceTreeTags(self, uiTreeTags, path, column, uiTextViewSrc):
        uiModel = uiTreeTags.get_model()
        line = uiModel.get_value(uiModel.get_iter(path), 1)
        self.ui_ScrollTextView(uiTextViewSrc, int(line))

    def ui_AddNotebookSourcePage(self, path, line):
        self.log.debug("Opening the file %s line %s" % (path, repr(line)))

        page_nr, v = self.lookup_notebook_source_page(path)
        if page_nr != None:
            self.uiNotebookSource.set_current_page(page_nr)
            self.ui_ScrollTextView(v[3], line)
            return

        tags = PyTags(self.log, self.conf, path)
        if len(tags.tags) == 0:
            self.log.debug("No tags to decode")
            return

        uiStoreTags = Gtk.ListStore(str, str, str)
        for tag in tags.tags:
            t = tags.decode(tag)[:4]
            uiStoreTags.append([t[0], t[2], t[3]])

        uiTreeTags = Gtk.TreeView(uiStoreTags)
        for i, column_title in enumerate(["Name", "Line", "Type"]):
            uiCell = Gtk.CellRendererText()
            uiCol = Gtk.TreeViewColumn(column_title, uiCell, text = i)
            uiCol.set_resizable(True)
            uiCol.set_sort_column_id(i)
            uiTreeTags.append_column(uiCol)

        uiHPan = Gtk.Paned(orientation = Gtk.Orientation.HORIZONTAL)
        uiHPan.set_position(int(self.uiPannedTop.get_allocated_width() *
                                float(self.conf["ui"]["tags-view-ratio"])))
        uiScrolled = Gtk.ScrolledWindow()
        uiScrolled.set_vexpand(True)
        uiScrolled.set_hexpand(True)
        uiScrolled.add(uiTreeTags)
        uiHPan.pack1(uiScrolled, True, False)

        uiTextViewSrc = Gtk.TextView()
        uiTextViewSrc.props.editable = False
        uiTextViewSrc.set_cursor_visible(True)
        uiTextViewSrc.props.left_margin = 10
        uiTextViewSrc.props.right_margin = 10
        uiTextViewSrc.props.top_margin = 10
        uiTextViewSrc.props.bottom_margin = 10
        uiTextViewSrc.modify_font(Pango.FontDescription(self.conf["fonts"]["source"]))

        uiTextViewSrc.connect('populate-popup', self.on_uiTextViewSrcPopUp)
        uiTextViewSrc.connect('key-press-event', self.on_uiTextViewSrcKeyPress)
        uiTextViewBuffer = uiTextViewSrc.get_buffer()
        with open(path, 'r') as f:
            uiTextViewBuffer.set_text(f.read())
            if line != None:
                self.ui_ScrollTextView(uiTextViewSrc, line)
        uiScrolled = Gtk.ScrolledWindow()
        uiScrolled.set_vexpand(True)
        uiScrolled.set_hexpand(True)
        uiScrolled.add(uiTextViewSrc)
        uiHPan.pack2(uiScrolled, True, False)

        uiHBox, uiCmdLock, uiLabel, uiCmdClose = self.ui_GenNotebookTitle(os.path.basename(path))
        uiHBox.show_all()

        uiTreeTags.connect("row-activated", self.on_uiSourceTreeTags, uiTextViewSrc)

        self.uiNotebookSource.append_page(uiHPan, uiHBox)
        page_nr = self.uiNotebookSource.page_num(uiHPan)
        self.uiNotebookSource.show_all()
        self.uiNotebookSource.set_current_page(page_nr)

        uiCmdLock.connect("clicked", self.on_uiNotebookLock, uiCmdLock)
        uiCmdClose.connect("clicked", self.on_uiNotebookClose,
                           (self.uiNotebookSource, uiCmdLock, page_nr,
                            self.notebook_source_pages, 'open', path, None))

        self.project_settings['entry'].append(self.gen_entry_open(path))
        self.notebook_source_pages[page_nr] = [path, uiStoreTags, uiTreeTags, uiTextViewSrc]

        self.ui_on_modify()

    def on_CscopeSymActivate(self, file, line):
        page_nr, v = self.lookup_notebook_source_page(file)
        if page_nr != None:
            self.uiNotebookSource.set_current_page(page_nr)
            self.ui_ScrollTextView(v[3], int(line))
        else:
            self.ui_AddNotebookSourcePage(file, int(line))

    def on_uiTreeCscope(self, uiTreeCscope, path, column, ignore):
        uiModel = uiTreeCscope.get_model()
        file = uiModel.get_value(uiModel.get_iter(path), 0)
        line = uiModel.get_value(uiModel.get_iter(path), 1)
        self.on_CscopeSymActivate(file, line)

    def ui_AddNotebookCscopePage(self, sym, sym_type):
        self.log.debug("Lookup for '%s' as '%d:%s'" %
                       (sym, sym_type, get_cscope_param_help(sym_type)))
        cscope = PyCscope(self.log, self.conf, sym, sym_type)
        if len(cscope.results) == 0:
            self.log.debug("No symbols to decode")
            return
        elif len(cscope.results) == 1:
            self.on_CscopeSymActivate(cscope.results[0][0], cscope.results[0][2])
            return
        uiStoreCscope = Gtk.ListStore(str, str, str)
        for i in cscope.results:
            uiStoreCscope.append([i[0], i[2], i[3]])
        uiTreeCscope = Gtk.TreeView(uiStoreCscope)
        for i, column_title in enumerate(["File", "Line", "Text"]):
            uiCell = Gtk.CellRendererText()
            uiCol = Gtk.TreeViewColumn(column_title, uiCell, text = i)
            uiCol.set_resizable(True)
            uiCol.set_sort_column_id(i)
            uiTreeCscope.append_column(uiCol)

        uiScrolled = Gtk.ScrolledWindow()
        uiScrolled.set_hexpand(True)
        uiScrolled.set_vexpand(True)
        uiScrolled.add(uiTreeCscope)

        uiHBox, uiCmdLock, uiLabel, uiCmdClose = self.ui_GenNotebookTitle(sym)
        uiHBox.show_all()

        uiTreeCscope.connect("row-activated", self.on_uiTreeCscope, None)

        self.uiNotebookCscope.append_page(uiScrolled, uiHBox)
        page_nr = self.uiNotebookSource.page_num(uiHBox)
        self.uiNotebookCscope.show_all()
        self.uiNotebookCscope.set_current_page(page_nr)

        uiCmdLock.connect("clicked", self.on_uiNotebookLock, uiCmdLock)
        uiCmdClose.connect("clicked", self.on_uiNotebookClose,
                           (self.uiNotebookCscope, uiCmdLock, page_nr,
                            None, 'cscope', sym, sym_type))

        self.project_settings['entry'].append(self.gen_entry_cscope(sym, sym_type))
        self.ui_on_modify()

    def on_uiMenuFileOpen(self, widget):
        uiDialog = Gtk.FileChooserDialog("Choose a file",
                                         self, Gtk.FileChooserAction.OPEN,
                                         (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                          Gtk.STOCK_OPEN, Gtk.ResponseType.OK))
        uiFilter = Gtk.FileFilter().new()
        uiFilter.set_name("All files")
        uiFilter.add_pattern('*.*')
        uiDialog.add_filter(uiFilter)
        uiDialog.set_current_folder(self.cwd)
        resp = uiDialog.run()
        filename = uiDialog.get_filename()
        uiDialog.destroy()

        if resp == Gtk.ResponseType.OK:
            if filename.startswith(self.cwd):
                filename = filename[len(self.cwd):]
            self.ui_AddNotebookSourcePage(filename, None)

    def on_uiMenuCscope(self, widget, data):
        title, query_type = data
        sym = ui_InputDialog(self, "Enter name for lookup", title)
        if sym != None:
            self.ui_AddNotebookCscopePage(sym, query_type)

    def on_uiMenuProjectSave(self, widget):
        if self.project_file == None:
            uiDialog = Gtk.FileChooserDialog("Choose a file",
                                             self, Gtk.FileChooserAction.SAVE,
                                             (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                              Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
            uiFilter = Gtk.FileFilter().new()
            uiFilter.set_name("JSON")
            uiFilter.add_pattern('*.json')
            uiDialog.add_filter(uiFilter)
            uiDialog.set_current_folder(self.cwd)
            resp = uiDialog.run()
            filename = uiDialog.get_filename()
            uiDialog.destroy()
            if resp == Gtk.ResponseType.OK:
                if filename.startswith(self.cwd):
                    filename = filename[len(self.cwd):]
                if not filename.endswith('.json'):
                    filename += '.json'
                self.save_project(filename)
        else:
            self.save_project(self.project_file)

    def on_uiMenuProjectClose(self, widget):
        if self.project_file == None:
            return
        self.project_file = None
        self.project_modified = False
        self.props.title = self.ui_gen_title()

    def on_uiMenuProjectOpen(self, widget):
        uiDialog = Gtk.FileChooserDialog("Choose a file",
                                         self, Gtk.FileChooserAction.OPEN,
                                         (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                          Gtk.STOCK_OPEN, Gtk.ResponseType.OK))
        uiFilter = Gtk.FileFilter().new()
        uiFilter.set_name("JSON")
        uiFilter.add_pattern('*.json')
        uiDialog.add_filter(uiFilter)
        uiDialog.set_current_folder(self.cwd)
        resp = uiDialog.run()
        filename = uiDialog.get_filename()
        uiDialog.destroy()
        if resp == Gtk.ResponseType.OK:
            if filename.startswith(self.cwd):
                filename = filename[len(self.cwd):]
            self.load_project(filename)

    def on_uiMenuFileExit(self, widget):
        Gtk.main_quit()
