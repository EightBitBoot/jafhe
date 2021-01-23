#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#define IN_RANGE(x, min, max) ((x >= min) && (x <= max))
#define CLAMP_VALUE(x, min, max) do{if(x < min){x = min;} else if(x > max){x = max;}} while(0);

#define LINE_LENGTH 16
#define DEFAULT_FONT "Monospace Normal 12"

#define HEX_BUFFER_LENGTH 48 // TODO(Adin): Update this later when lines are resizable
#define HEX_BUFFER_OFFSET(x) (x * 2 + (x - 1))

#define ASCII_BUFFER_LENGTH 17

#define BOX_SPACING_PX 6
#define TEXT_MARGIN_PX 2

typedef uint8_t byte;

struct _ProgramState {
    GtkWidget *window;

    GtkWidget *closeMenuI;
    GtkWidget *gotoMenuI;

    GtkWidget *viewWidgetsBox;
    GtkWidget *offsetBox;
    GtkWidget *hexBox;
    GtkWidget *asciiBox;
    GtkWidget *scrollBar;

    GtkAdjustment *scrollAdj;

    PangoFontDescription *fontDesc;
    uint fontWidth;
    uint fontHeight;

    uint widgetHeight;
    uint numLines;

    // TODO(Adin): Make this resizable for different line lengths later
    char hexLineBuffer[HEX_BUFFER_LENGTH];

    char asciiLineBuffer[ASCII_BUFFER_LENGTH];

    FILE *file;
    char *fileFullName;
    byte *fileBuffer;
    ulong fileLength;
    uint  fileNumLines;
};
typedef struct _ProgramState ProgramState;

static ProgramState state = {0};

int jceil(double value);

void shutdownAndCleanup();
void closeCurrentFile(bool performUpdates);

void openFile(char *filename);
void openMenuAction(GtkMenuItem *menuItem);
void gotoActivateCallback(GtkWidget *widget, gpointer data);
void openGotoDialog();
void gotoMenuAction(GtkWidget *widget);
void fontMenuAction(GtkMenuItem *menuItem);

bool onKeyPress(GtkWidget *widget, GdkEventKey *event);
void onUpdateSize(GtkWidget *widget, GdkRectangle *newRectangle);
void onAdjValueChanged(GtkAdjustment *adj);
void onScrollEvent(GtkWidget *widget, GdkEvent *event);

void updateTitle();
uint getFontWidth(GtkWidget *widget, PangoFontDescription *fontDesc);
void updateFont(PangoFontDescription *newDesc);
void updateSizeRequests();
void updateSizeRequests();

void fillHexBuffer(ulong offset);
void fillAsciiBuffer(ulong offset);
gboolean renderOffsetBox(GtkWidget *widget, cairo_t *cr);
gboolean renderHexBox(GtkWidget *widget, cairo_t *cr);
gboolean renderAsciiBox(GtkWidget *widget, cairo_t *cr);

void toggleMenuSensitivity();
bool accelCallback(GtkAccelGroup *group, GObject *obj, guint keyval, GdkModifierType modifier, gpointer data);
GtkWidget *buildMenu();

int main(int argc, char **argv);

int jceil(double value) {
    if(value > (int) value) {
        return (int) value + 1;
    }

    return (int) value;
}

void shutdownAndCleanup() {
    // TODO(Adin): Close files and do cleanup here
    closeCurrentFile(false);

    gtk_main_quit();
}

void closeCurrentFile(bool performUpdates) {
    if(state.file != NULL) {
        fclose(state.file);
        state.file = NULL;
    }
    
    if(state.fileFullName != NULL) {
        free(state.fileFullName);
        state.fileFullName = NULL;
    }

    if(state.fileBuffer != NULL) {
        free(state.fileBuffer);
        state.fileBuffer = NULL;
    }

    state.fileLength = 0;
    state.fileNumLines = 0;
    
    if(performUpdates) {
        updateTitle();
        toggleMenuSensitivity();
    }


    updateSizeRequests();

    if(state.viewWidgetsBox) {
        gtk_widget_queue_draw(state.viewWidgetsBox);
    }
}

void openFile(char *filename) {
    closeCurrentFile(false);

    state.fileFullName = malloc(strlen(filename) + 1);
    memcpy(state.fileFullName, filename, strlen(filename) + 1); // + 1 to copy the implicit null terminator

    state.file = fopen(state.fileFullName, "r");

    fseek(state.file, 0, SEEK_END);
    state.fileLength = ftell(state.file);
    fseek(state.file, 0, SEEK_SET);

    state.fileBuffer = malloc(state.fileLength + 1);
    memset(state.fileBuffer, 0, state.fileLength + 1);
    fread(state.fileBuffer, 1, state.fileLength, state.file);

    state.fileNumLines = jceil((float) state.fileLength / (float) LINE_LENGTH);

    toggleMenuSensitivity();
    updateSizeRequests();

    if(state.viewWidgetsBox) {
        gtk_widget_queue_draw(state.viewWidgetsBox);
    }
}

void openMenuAction(GtkMenuItem *menuItem) {
    GtkWidget *dialog = NULL;
    gint dialogResult = 0;

    dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(state.window), GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);

    dialogResult = gtk_dialog_run(GTK_DIALOG(dialog));
    if(dialogResult == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        openFile(filename);
        g_free(filename);
    }

    updateTitle();

    gtk_widget_destroy(dialog);
}

void gotoActivateCallback(GtkWidget *widget, gpointer data) {
    gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT);
}

void openGotoDialog() {
    GtkWidget *dialog = NULL;
    GtkWidget *dialogCBox = NULL;
    GtkWidget *entry = NULL;

    bool done = FALSE;
    gint response = 0;

    dialog = gtk_dialog_new_with_buttons("Jump To Offset", GTK_WINDOW(state.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_set_events(dialog, GDK_KEY_PRESS_MASK);

    entry = gtk_entry_new();
    g_signal_connect(entry, "activate", G_CALLBACK(gotoActivateCallback), dialog); // Done this way because gtk_dialog_add_action_widget doesn't working
    dialogCBox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(dialogCBox), entry, FALSE, FALSE, 0);
    gtk_widget_show(entry);

    while(!done) {
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if(response == GTK_RESPONSE_ACCEPT) {
            const char *entryText = gtk_entry_get_text(GTK_ENTRY(entry));
            char *rest = NULL;
            long offset = 0;

            offset = strtol(entryText, &rest, 16);

            if(strlen(rest) != 0) {
                GtkWidget *invalidDialog = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Invalid Offset: \"%s\"", entryText);
                gtk_dialog_run(GTK_DIALOG(invalidDialog));
                gtk_widget_destroy(invalidDialog);
                gtk_entry_set_text(GTK_ENTRY(entry), "");
                gtk_widget_grab_focus(GTK_WIDGET(entry));
            } 
            else {
                // Success
                // TODO(Adin): Update for resizable lines
                offset >>= 4; // Divide by line length (2^4 or 16)
                int adjUpper = gtk_adjustment_get_upper(state.scrollAdj);
                int adjPSize = gtk_adjustment_get_page_size(state.scrollAdj);
                CLAMP_VALUE(offset, 0, adjUpper - adjPSize);
                gtk_adjustment_set_value(state.scrollAdj, offset);
                done = TRUE;
            }
            
        }
        else {
            done = TRUE;
        }
    }

    gtk_widget_destroy(dialog);
}

void gotoMenuAction(GtkWidget *widget) {
    if(state.file) {
        openGotoDialog();
    }
}

void fontMenuAction(GtkMenuItem *menuItem) {
    GtkWidget *dialog = NULL;
    gint dialogResult = 0;

    dialog = gtk_font_chooser_dialog_new("Choose Font", GTK_WINDOW(state.window));
    gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(dialog), state.fontDesc);

    dialogResult = gtk_dialog_run(GTK_DIALOG(dialog));
    if(dialogResult == GTK_RESPONSE_OK) {
        PangoFontDescription *fontDesc = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));

        updateFont(pango_font_description_copy(fontDesc));
    }

    gtk_widget_destroy(dialog);
}

bool onKeyPress(GtkWidget *widget, GdkEventKey *event) {
    switch(event->keyval) {
        case GDK_KEY_k:
        case GDK_KEY_Up:
            if(state.scrollAdj && state.file) {
                int newValue = gtk_adjustment_get_value(state.scrollAdj) - 1;

                if(newValue < 0) {
                    newValue = 0;
                }

                gtk_adjustment_set_value(state.scrollAdj, newValue);
            }
            break;

        case GDK_KEY_j:
        case GDK_KEY_Down:
            if(state.scrollAdj && state.file) {
                int newValue = gtk_adjustment_get_value(state.scrollAdj) + 1;
                int upper = gtk_adjustment_get_upper(state.scrollAdj);
                int pSize = gtk_adjustment_get_page_size(state.scrollAdj);

                if(newValue > upper - pSize) {
                    newValue = upper - pSize;
                }

                gtk_adjustment_set_value(state.scrollAdj, newValue);
            }
            break;
    }

    return FALSE;
}

void onUpdateSize(GtkWidget *widget, GdkRectangle *newRectangle) {
    if(newRectangle) {
        state.widgetHeight = newRectangle->height; 
    }

    if(state.fontHeight != 0) {
        state.numLines = state.widgetHeight / state.fontHeight;
        if(state.widgetHeight % state.fontHeight) {
            state.numLines++;
        }
    }

    if(state.scrollAdj) {
        uint value = gtk_adjustment_get_value(state.scrollAdj);
        gtk_adjustment_configure(state.scrollAdj, value, 0, (state.widgetHeight % state.fontHeight ? state.fileNumLines + 1 : state.fileNumLines), 1, 1, state.numLines); // Ternary adjusts for rendering cutoff last line in file
    }
}

void onAdjValueChanged(GtkAdjustment *adj) {
    if(state.viewWidgetsBox) {
        gtk_widget_queue_draw(state.viewWidgetsBox);
    }
}

void onScrollEvent(GtkWidget *widget, GdkEvent *event) {
    gtk_widget_event(state.scrollBar, event);
}

void updateTitle() {
    char titleBuffer[39] = {0}; // 50 bytes for the file + 8 bytes for "JAFHE - " + 1 byte for terminator

    if(state.fileFullName != NULL) {
        char *fileName = rindex(state.fileFullName, '/') + 1;

        if(strlen(fileName) > 30) {
            snprintf(titleBuffer, 39, "JAFHE - %.27s...", fileName);
        }
        else {
            snprintf(titleBuffer, 39, "JAFHE - %s", fileName);
        }
        gtk_window_set_title(GTK_WINDOW(state.window), titleBuffer);
    }
    else {
        gtk_window_set_title(GTK_WINDOW(state.window), "JAFHE");
    }
}

uint getFontWidth(GtkWidget *widget, PangoFontDescription *fontDesc) {
    PangoLayout *layout = gtk_widget_create_pango_layout(widget, "");
    PangoRectangle rect = {0};

    uint maxWidth = 0;
    char str[2] = {0};

    pango_layout_set_font_description(layout, fontDesc);

    for(int i = 0x20; i <= 0x7E; i++) {
        snprintf(str, 2, "%c", (char) i);
        pango_layout_set_text(layout,  str, -1);
        pango_layout_get_pixel_extents(layout, NULL, &rect);

        maxWidth = MAX(rect.width, maxWidth);
    }

    g_object_unref(G_OBJECT(layout));

    return maxWidth;
}

void updateFont(PangoFontDescription *newDesc) {
    GtkWidget *temp = NULL;

    PangoContext *hexPangoContext = NULL;
    PangoFontMetrics *fontMetrics = NULL;

    pango_font_description_free(state.fontDesc);
    state.fontDesc = newDesc;

    temp = gtk_text_view_new();

    hexPangoContext = gtk_widget_get_pango_context(temp);
    fontMetrics = pango_context_get_metrics(hexPangoContext, state.fontDesc, NULL);

    state.fontHeight = PANGO_PIXELS(pango_font_metrics_get_ascent(fontMetrics)) + PANGO_PIXELS(pango_font_metrics_get_descent(fontMetrics)) + 2;
    state.fontWidth = getFontWidth(temp, state.fontDesc);

    gtk_widget_destroy(temp);
    
    updateSizeRequests();

    if(state.viewWidgetsBox) {
        gtk_widget_queue_draw(state.viewWidgetsBox);
    }
}

void updateSizeRequests() {
    int height = -1;

    if(state.file) {
        height = MIN(10, state.fileNumLines) * state.fontHeight;
    }

    if(state.viewWidgetsBox) {
        gtk_widget_set_size_request(state.viewWidgetsBox, -1, height);
    }

    if(state.offsetBox) {
        gtk_widget_set_size_request(state.offsetBox, 8 * state.fontWidth + 2 * TEXT_MARGIN_PX, height);
    }

    if(state.hexBox) {
        gtk_widget_set_size_request(state.hexBox, (HEX_BUFFER_LENGTH - 1) * state.fontWidth + 2 * TEXT_MARGIN_PX, height);
    }

    if(state.asciiBox) {
        gtk_widget_set_size_request(state.asciiBox, (ASCII_BUFFER_LENGTH - 1) * state.fontWidth + 2 * TEXT_MARGIN_PX, height);
    }
}

void fillHexBuffer(ulong offset) {
    // TODO(Adin): Update when lines are resizable

    for(long i = 0; i < MIN(LINE_LENGTH, state.fileLength - offset); i++) {
        if(i == 0) {
            snprintf(state.hexLineBuffer, HEX_BUFFER_LENGTH, "%02X", state.fileBuffer[offset + i]);
        }
        else {
            snprintf(state.hexLineBuffer + HEX_BUFFER_OFFSET(i), HEX_BUFFER_LENGTH - HEX_BUFFER_OFFSET(i), " %02X", state.fileBuffer[offset + i]);
        }
    }
} 

void fillAsciiBuffer(ulong offset) {
    // TODO(Adin): Update when lines are resizable
    char current = 0;

    for(long i = 0; i < MIN(LINE_LENGTH, state.fileLength - offset); i++) {
        if(IN_RANGE(state.fileBuffer[offset + i], 0x20, 0x7E)) {
            current = state.fileBuffer[offset + i];
        }
        else {
            current = '.';
        }

        snprintf(state.asciiLineBuffer + i, ASCII_BUFFER_LENGTH - i, "%c", current);
    }
}

gboolean renderOffsetBox(GtkWidget *widget, cairo_t *cr) {
    if(!state.file) {
        // If there isn't an open file don't render the box
        return FALSE;
    }

    GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
    GtkStateFlags widgetState = gtk_style_context_get_state(styleContext);

    PangoContext *pangoContext = gtk_widget_get_pango_context(widget);
    PangoLayout *pangoLayout = pango_layout_new(pangoContext);

    GdkRGBA fgColor = {0};

    char buffer[9] = {0}; // 2 bytes "0x" + 8 bytes of hex + null terminator

    gtk_style_context_get_color(styleContext, widgetState, &fgColor);
    
    uint width = gtk_widget_get_allocated_width(widget);
    uint height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(styleContext, cr, 0, 0, width, height);
    
    gdk_cairo_set_source_rgba(cr, &fgColor);
    pango_layout_set_font_description(pangoLayout, state.fontDesc);

    uint adjValue = gtk_adjustment_get_value(state.scrollAdj);
    uint linesToDraw = MIN(state.numLines, state.fileNumLines - adjValue);
    for(int i = 0; i < linesToDraw; i++) {
        snprintf(buffer, 9, "%08X", (adjValue * LINE_LENGTH) + i * LINE_LENGTH);
        pango_layout_set_text(pangoLayout, buffer, 10);

        cairo_move_to(cr, TEXT_MARGIN_PX, i * state.fontHeight);
        pango_cairo_show_layout(cr, pangoLayout);
    }

    g_object_unref(G_OBJECT(pangoLayout));

    return FALSE;
}

gboolean renderHexBox(GtkWidget *widget, cairo_t *cr) {
    if(!state.file) {
        // If there isn't an open file don't render the box
        return FALSE;
    }

    GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
    GtkStateFlags widgetState = gtk_style_context_get_state(styleContext);

    PangoContext *pangoContext = gtk_widget_get_pango_context(widget);
    PangoLayout *pangoLayout = pango_layout_new(pangoContext); 

    GdkRGBA fgColor = {0};
    
    gtk_style_context_get_color(styleContext, widgetState, &fgColor);

    uint width = gtk_widget_get_allocated_width(widget);
    uint height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(styleContext, cr, 0, 0, width, height);

    gdk_cairo_set_source_rgba(cr, &fgColor);
    pango_layout_set_font_description(pangoLayout, state.fontDesc);

    uint adjValue = gtk_adjustment_get_value(state.scrollAdj);
    uint linesToDraw = MIN(state.numLines, state.fileNumLines - adjValue);
    for(int i = 0; i < linesToDraw; i++) {
        fillHexBuffer((adjValue * LINE_LENGTH) + i * LINE_LENGTH);
        pango_layout_set_text(pangoLayout, state.hexLineBuffer, -1);

        cairo_move_to(cr, TEXT_MARGIN_PX, i * state.fontHeight);
        pango_cairo_show_layout(cr, pangoLayout);
    }

    g_object_unref(G_OBJECT(pangoLayout));

    return FALSE;
}

gboolean renderAsciiBox(GtkWidget *widget, cairo_t *cr) {
    if(!state.file) {
        // If there isn't an open file don't render the box
        return FALSE;
    }

    GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
    GtkStateFlags widgetState = gtk_style_context_get_state(styleContext);

    PangoContext *pangoContext = gtk_widget_get_pango_context(widget);
    PangoLayout *pangoLayout = pango_layout_new(pangoContext); 

    GdkRGBA fgColor = {0};
    
    gtk_style_context_get_color(styleContext, widgetState, &fgColor);

    uint width = gtk_widget_get_allocated_width(widget);
    uint height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(styleContext, cr, 0, 0, width, height);

    gdk_cairo_set_source_rgba(cr, &fgColor);
    pango_layout_set_font_description(pangoLayout, state.fontDesc);

    uint adjValue = gtk_adjustment_get_value(state.scrollAdj);
    uint linesToDraw = MIN(state.numLines, state.fileNumLines - adjValue);
    for(int i = 0; i < linesToDraw; i++) {
        fillAsciiBuffer((adjValue * LINE_LENGTH) + i * LINE_LENGTH);
        pango_layout_set_text(pangoLayout, state.asciiLineBuffer, -1);

        cairo_move_to(cr, TEXT_MARGIN_PX, i * state.fontHeight);
        pango_cairo_show_layout(cr, pangoLayout);
    }

    g_object_unref(G_OBJECT(pangoLayout));

    return FALSE;
}

void toggleMenuSensitivity() {
    bool sensitivity = FALSE;

    if(state.file) {
        sensitivity = TRUE;
    }
    else {
        sensitivity = FALSE;
    }

    gtk_widget_set_sensitive(state.closeMenuI, sensitivity);
    gtk_widget_set_sensitive(state.gotoMenuI,  sensitivity);
}

bool accelCallback(GtkAccelGroup *group, GObject *obj, guint keyval, GdkModifierType modifier, gpointer data) {
    return FALSE; // Allow other handlers to process the signal
}

GtkWidget *buildMenu() {
    GtkWidget *menubar =     NULL;

    GtkAccelGroup *accelGroup = NULL;
    GClosure *openClosure = NULL;
    GClosure *closeClosure = NULL;
    GClosure *gotoClosure = NULL;
    GClosure *quitClosure = NULL;

    GtkWidget *fileMenu =    NULL;
    GtkWidget *fileMenuI =   NULL;

    GtkWidget *openMenuI =   NULL;
    GtkWidget *fontMenuI =   NULL;
    GtkWidget *quitMenuI =   NULL;

    menubar =     gtk_menu_bar_new();
    fileMenu =    gtk_menu_new();
    fileMenuI =   gtk_menu_item_new_with_label("File");

    openMenuI =        gtk_menu_item_new_with_label("Open");
    state.closeMenuI = gtk_menu_item_new_with_label("Close");
    state.gotoMenuI =  gtk_menu_item_new_with_label("Goto");
    fontMenuI =        gtk_menu_item_new_with_label("Font");
    quitMenuI =        gtk_menu_item_new_with_label("Quit");

    g_signal_connect(G_OBJECT(openMenuI),        "activate", G_CALLBACK(openMenuAction),     NULL);
    g_signal_connect(G_OBJECT(state.closeMenuI), "activate", G_CALLBACK(closeCurrentFile),   NULL);
    g_signal_connect(G_OBJECT(state.gotoMenuI),  "activate", G_CALLBACK(gotoMenuAction),     NULL);
    g_signal_connect(G_OBJECT(fontMenuI),        "activate", G_CALLBACK(fontMenuAction),     NULL);
    g_signal_connect(G_OBJECT(quitMenuI),        "activate", G_CALLBACK(shutdownAndCleanup), NULL);

    gtk_accel_map_add_entry("<JAFHE>/File/Open",  GDK_KEY_O, GDK_CONTROL_MASK);
    gtk_accel_map_add_entry("<JAFHE>/File/Close", GDK_KEY_W, GDK_CONTROL_MASK);
    gtk_accel_map_add_entry("<JAFHE>/File/Goto",  GDK_KEY_G, GDK_CONTROL_MASK);
    gtk_accel_map_add_entry("<JAFHE>/File/Quit",  GDK_KEY_Q, GDK_CONTROL_MASK);

    accelGroup = gtk_accel_group_new();

    openClosure =  g_cclosure_new(G_CALLBACK(accelCallback), openMenuI,        0);
    closeClosure = g_cclosure_new(G_CALLBACK(accelCallback), state.closeMenuI, 0);
    gotoClosure =  g_cclosure_new(G_CALLBACK(accelCallback), state.gotoMenuI,  0);
    quitClosure =  g_cclosure_new(G_CALLBACK(accelCallback), quitMenuI,        0);

    gtk_accel_group_connect_by_path(accelGroup, "<JAFHE>/File/Open",  openClosure);
    gtk_accel_group_connect_by_path(accelGroup, "<JAFHE>/File/Close", closeClosure);
    gtk_accel_group_connect_by_path(accelGroup, "<JAFHE>/File/Goto",  gotoClosure);
    gtk_accel_group_connect_by_path(accelGroup, "<JAFHE>/File/Quit",  quitClosure);

    gtk_window_add_accel_group(GTK_WINDOW(state.window), accelGroup);
    gtk_menu_set_accel_group(GTK_MENU(fileMenu), accelGroup);

    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(openMenuI),        "<JAFHE>/File/Open");
    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(state.closeMenuI), "<JAFHE>/File/Close");
    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(state.gotoMenuI),  "<JAFHE>/File/Goto");
    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(quitMenuI),        "<JAFHE>/File/Quit");

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMenuI);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenuI), fileMenu);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), state.closeMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), state.gotoMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fontMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMenuI);

    toggleMenuSensitivity();

    return menubar;
}

int main(int argc, char **argv) {
    GtkWidget *menubar = NULL;

    GtkWidget *vbox = NULL;

    GtkStyleContext *hexStyleContext = NULL;
    GtkStyleContext *asciiStyleContext = NULL;

    PangoFontDescription *defaultFontDesc = NULL;

    gtk_init(&argc, &argv);

    state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state.window), "JAFHE");
    gtk_window_set_default_size(GTK_WINDOW(state.window), 600, 400);
    gtk_window_set_icon_from_file(GTK_WINDOW(state.window), "jafhe.svg", NULL); // Does this even do anything?
    gtk_widget_set_events(state.window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(state.window), "destroy", G_CALLBACK(shutdownAndCleanup), NULL);
    g_signal_connect(G_OBJECT(state.window), "key-press-event", G_CALLBACK(onKeyPress), NULL);

    defaultFontDesc = pango_font_description_from_string(DEFAULT_FONT);
    updateFont(defaultFontDesc);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), vbox);

    menubar = buildMenu();

    state.viewWidgetsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_spacing(GTK_BOX(state.viewWidgetsBox), BOX_SPACING_PX);

    state.offsetBox = gtk_drawing_area_new();
    g_signal_connect(state.offsetBox, "draw", G_CALLBACK(renderOffsetBox), NULL);

    state.hexBox = gtk_drawing_area_new();
    hexStyleContext = gtk_widget_get_style_context(state.hexBox);
    gtk_style_context_add_class(hexStyleContext, GTK_STYLE_CLASS_VIEW);
    gtk_widget_set_events(state.hexBox, GDK_SCROLL_MASK);
    g_signal_connect(state.hexBox, "size-allocate", G_CALLBACK(onUpdateSize), NULL);
    g_signal_connect(state.hexBox, "draw", G_CALLBACK(renderHexBox), NULL);
    g_signal_connect(state.hexBox, "scroll-event", G_CALLBACK(onScrollEvent), NULL);

    state.asciiBox = gtk_drawing_area_new();
    asciiStyleContext = gtk_widget_get_style_context(state.asciiBox);
    gtk_style_context_add_class(asciiStyleContext, GTK_STYLE_CLASS_VIEW);
    gtk_widget_set_events(state.asciiBox, GDK_SCROLL_MASK);
    g_signal_connect(state.asciiBox, "draw", G_CALLBACK(renderAsciiBox), NULL);
    g_signal_connect(state.asciiBox, "scroll-event", G_CALLBACK(onScrollEvent), NULL);

    state.scrollAdj = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    g_signal_connect(state.scrollAdj, "value-changed", G_CALLBACK(onAdjValueChanged), NULL);
    state.scrollBar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, state.scrollAdj);

    updateSizeRequests();

    gtk_box_pack_start(GTK_BOX(state.viewWidgetsBox), state.offsetBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(state.viewWidgetsBox), state.hexBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(state.viewWidgetsBox), state.asciiBox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(state.viewWidgetsBox), state.scrollBar, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), state.viewWidgetsBox, TRUE, TRUE, 0);

    gtk_widget_show_all(state.window);

    gtk_main();

    return 0;
}