#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#define IN_RANGE(x, min, max) ((x >= min) && (x <= max))

#define LINE_LENGTH 16
#define DEFAULT_FONT "Monospace Normal 12"

#define HEX_BUFFER_LENGTH 48
#define HEX_BUFFER_OFFSET(x) (x * 2 + (x - 1))

#define ASCII_BUFFER_LENGTH 17

#define BOX_SPACING_PX 6
#define TEXT_MARGIN_PX 2

typedef uint8_t byte;

struct _ProgramState {
    GtkWidget *window;

    GtkWidget *fileWidgetsBox;
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
    ulong fileLength;
    uint  fileNumLines;
    byte *fileBuffer;
};
typedef struct _ProgramState ProgramState;

static ProgramState state = {0};

void closeCurrentFile(bool titleUpdateNeeded);
void updateTitle();
void updateSizeRequests();

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

void closeCurrentFile(bool titleUpdateNeeded) {
    if(state.fileBuffer != NULL) {
        free(state.fileBuffer);
        state.fileBuffer = NULL;
    }
    if(state.file != NULL) {
        fclose(state.file);
        state.file = NULL;
    }
    if(state.fileFullName != NULL) {
        free(state.fileFullName);
        state.fileFullName = NULL;
    }

    if(titleUpdateNeeded) {
        updateTitle();
    }

    state.fileLength = 0;
    state.fileNumLines = 0;

    updateSizeRequests();

    if(state.fileWidgetsBox) {
        gtk_widget_queue_draw(state.fileWidgetsBox);
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

    updateSizeRequests();

    if(state.fileWidgetsBox) {
        gtk_widget_queue_draw(state.fileWidgetsBox);
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

void updateSizeRequests() {
    int height = -1;

    if(state.file) {
        height = MIN(10, state.fileNumLines) * state.fontHeight;
    }

    if(state.fileWidgetsBox) {
        gtk_widget_set_size_request(state.fileWidgetsBox, -1, height);
    }

    if(state.offsetBox) {
        gtk_widget_set_size_request(state.offsetBox, 8 * state.fontWidth + 2 * TEXT_MARGIN_PX, height);
        // printf("OffsetBox: %d\n", state.offsetBox, 8 * state.fontWidth + 2 * TEXT_MARGIN_PX);
    }

    if(state.hexBox) {
        gtk_widget_set_size_request(state.hexBox, (HEX_BUFFER_LENGTH - 1) * state.fontWidth + 2 * TEXT_MARGIN_PX, height);
        // printf("HexBox: %d\n", state.offsetBox, (HEX_BUFFER_LENGTH - 1) * state.fontWidth + 2 * TEXT_MARGIN_PX);
    }

    if(state.asciiBox) {
        gtk_widget_set_size_request(state.asciiBox, (ASCII_BUFFER_LENGTH - 1) * state.fontWidth + 2 * TEXT_MARGIN_PX, height);
        // printf("AsciiBox: %d\n", (ASCII_BUFFER_LENGTH - 1) * state.fontWidth + 2 * TEXT_MARGIN_PX);
    }
}

void updateFont(PangoFontDescription *newDesc) {
    GtkWidget *temp = NULL;

    PangoContext *hexPangoContext = NULL;
    PangoFontMetrics *fontMetrics = NULL;

    pango_font_description_free(state.fontDesc);
    state.fontDesc = newDesc;

    // TODO(Adin): Update font metrics 
    temp = gtk_text_view_new();

    hexPangoContext = gtk_widget_get_pango_context(temp);
    fontMetrics = pango_context_get_metrics(hexPangoContext, state.fontDesc, NULL);

    state.fontHeight = PANGO_PIXELS(pango_font_metrics_get_ascent(fontMetrics)) + PANGO_PIXELS(pango_font_metrics_get_descent(fontMetrics)) + 2;
    state.fontWidth = getFontWidth(temp, state.fontDesc);

    gtk_widget_destroy(temp);
    
    printf("Selected Font Family: %s, Width: %u, Height: %u\n", pango_font_description_get_family(state.fontDesc), state.fontWidth, state.fontHeight);

    updateSizeRequests();

    if(state.fileWidgetsBox) {
        gtk_widget_queue_draw(state.fileWidgetsBox);
    }
}

void onScrollEvent(GtkWidget *widget, GdkEvent *event) {
    gtk_widget_event(state.scrollBar, event);
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
        gtk_adjustment_configure(state.scrollAdj, value, 0, state.fileNumLines, 1, 1, state.numLines);
    }
}

void fillHexBuffer(ulong offset) {
    // TODO(Adin): Update for when ending lines in a file are less than a full line
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
    // TODO(Adin): Update for when ending lines in a file are less than a full line
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

    uint linesToDraw = MIN(state.numLines, state.fileNumLines); // TODO(Adin): Update for scrolling
    uint adjValue = gtk_adjustment_get_value(state.scrollAdj);
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

    // cairo_arc(cr, width / 2.0, height / 2.0, MIN(width, height) / 2.0, 0, 2 * G_PI);
    // gdk_cairo_set_source_rgba(cr, &fgColor);
    // cairo_fill(cr);

    gdk_cairo_set_source_rgba(cr, &fgColor);
    pango_layout_set_font_description(pangoLayout, state.fontDesc);

    uint linesToDraw = MIN(state.numLines, state.fileNumLines); // TODO(Adin): Update for scrolling
    uint adjValue = gtk_adjustment_get_value(state.scrollAdj);
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

    uint linesToDraw = MIN(state.numLines, state.fileNumLines); // TODO(Adin): Update for scrolling
    uint adjValue = gtk_adjustment_get_value(state.scrollAdj);
    for(int i = 0; i < linesToDraw; i++) {
        fillAsciiBuffer((adjValue * LINE_LENGTH) + i * LINE_LENGTH);
        pango_layout_set_text(pangoLayout, state.asciiLineBuffer, -1);

        cairo_move_to(cr, TEXT_MARGIN_PX, i * state.fontHeight);
        pango_cairo_show_layout(cr, pangoLayout);
    }

    g_object_unref(G_OBJECT(pangoLayout));

    return FALSE;
}

void onAdjValueChanged(GtkAdjustment *adj) {
    printf("%f\n", gtk_adjustment_get_value(adj));
    if(state.fileWidgetsBox) {
        gtk_widget_queue_draw(state.fileWidgetsBox);
    }
}

GtkWidget *buildMenu() {
    GtkWidget *menubar =     NULL;

    GtkWidget *fileMenu =    NULL;
    GtkWidget *fileMenuI =   NULL;

    GtkWidget *openMenuI =   NULL;
    GtkWidget *closeMenuI =  NULL;
    GtkWidget *fontMenuI =   NULL;

    menubar =     gtk_menu_bar_new();
    fileMenu =    gtk_menu_new();
    fileMenuI =   gtk_menu_item_new_with_label("File");

    openMenuI =   gtk_menu_item_new_with_label("Open");
    closeMenuI =  gtk_menu_item_new_with_label("Close");
    fontMenuI =   gtk_menu_item_new_with_label("Font");

    g_signal_connect(G_OBJECT(openMenuI),   "activate", G_CALLBACK(openMenuAction),   NULL);
    g_signal_connect(G_OBJECT(closeMenuI),  "activate", G_CALLBACK(closeCurrentFile), NULL);
    g_signal_connect(G_OBJECT(fontMenuI),   "activate", G_CALLBACK(fontMenuAction),   NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMenuI);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenuI), fileMenu);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), closeMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fontMenuI);

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
    g_signal_connect(G_OBJECT(state.window), "destroy", G_CALLBACK(shutdownAndCleanup), NULL);

    defaultFontDesc = pango_font_description_from_string(DEFAULT_FONT);
    updateFont(defaultFontDesc);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), vbox);

    menubar = buildMenu();

    state.fileWidgetsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_spacing(GTK_BOX(state.fileWidgetsBox), BOX_SPACING_PX);

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

    state.scrollAdj = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 1.0, 20.0); // TODO(Adin): Change
    g_signal_connect(state.scrollAdj, "value-changed", G_CALLBACK(onAdjValueChanged), NULL);
    state.scrollBar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, state.scrollAdj);

    updateSizeRequests();

    gtk_box_pack_start(GTK_BOX(state.fileWidgetsBox), state.offsetBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(state.fileWidgetsBox), state.hexBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(state.fileWidgetsBox), state.asciiBox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(state.fileWidgetsBox), state.scrollBar, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), state.fileWidgetsBox, TRUE, TRUE, 0);

    gtk_widget_show_all(state.window);
    // gtk_widget_hide(state.scrollBar);

    gtk_main();

    return 0;
}