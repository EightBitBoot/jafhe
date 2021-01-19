#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <gtk/gtk.h>

#define REG_LINE_LEN 63

typedef uint8_t byte;

struct _ProgramState {
    GtkWidget *window;
    GtkWidget *fileWidgetsBox;

    PangoFontDescription *fontDesc;

    // TODO(Adin): Make this resizable for different line lengths later
    uint hexLineBufferLen;
    char hexLineBuffer[48];

    FILE *file;
    char *fileFullName;
    ulong fileLength;
    byte *fileBuffer;
};
typedef struct _ProgramState ProgramState;

static ProgramState state = {0};

void closeCurrentFile(bool titleUpdateNeeded);
void updateTitle();

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

    gtk_widget_queue_draw(state.fileWidgetsBox);
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

    gtk_widget_queue_draw(state.fileWidgetsBox);
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

void updateFont(PangoFontDescription *newDesc) {
    pango_font_description_free(state.fontDesc);
    state.fontDesc = newDesc;

    // TODO(Adin): Update font metrics (height, width)
    printf("Selected Font Family: %s\n", pango_font_description_get_family(state.fontDesc));

    if(state.fileWidgetsBox) {
        gtk_widget_queue_draw(state.fileWidgetsBox);
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

void resizeMenuAction(GtkMenuItem *menuItem) {
    gtk_widget_set_size_request(state.fileWidgetsBox, 300, 100);
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

#define HEX_BUFFER_LENGTH 48
#define HEX_BUFFER_OFFSET(x) (x * 2 + (x - 1))

void fillHexBuffer(ulong offset) {
    // TODO(Adin): Update for when ending lines in a file are less than a full line
    // TODO(Adin): Update when lines are resizable

    for(int i = 0; i < 16; i++) {
        if(i == 0) {
            snprintf(state.hexLineBuffer, HEX_BUFFER_LENGTH, "%02X", state.fileBuffer[offset + i]);
        }
        else {
            snprintf(state.hexLineBuffer + HEX_BUFFER_OFFSET(i), HEX_BUFFER_LENGTH - HEX_BUFFER_OFFSET(i), " %02X", state.fileBuffer[offset + i]);
        }
    }
} 

gboolean renderHexBox(GtkWidget *widget, cairo_t *cr) {
    GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
    GtkStateFlags widgetState = gtk_style_context_get_state(styleContext);

    PangoContext *pangoContext = gtk_widget_get_pango_context(widget);
    PangoFontMetrics *fontMetrics = pango_context_get_metrics(pangoContext, state.fontDesc, NULL);    
    PangoLayout *pangoLayout = pango_layout_new(pangoContext); 

    GdkRGBA fgColor = {0};
    GdkRGBA pink    = {0};
    
    gtk_style_context_get_color(styleContext, widgetState, &fgColor);
    gdk_rgba_parse(&pink, "#FF24FE");

    uint width = gtk_widget_get_allocated_width(widget);
    uint height = gtk_widget_get_allocated_height(widget);

    uint fontHeight = PANGO_PIXELS(pango_font_metrics_get_ascent(fontMetrics)) + PANGO_PIXELS(pango_font_metrics_get_descent(fontMetrics)) + 2;
    uint numLines = height / fontHeight;
    if(height % fontHeight) {
        numLines++;
    }

    char *hexStr = "0B AA FA AF 76 67 69 DB DD BB AF 76 73 00 80 90";

    gtk_render_background(styleContext, cr, 0, 0, width, height);

    // cairo_arc(cr, width / 2.0, height / 2.0, MIN(width, height) / 2.0, 0, 2 * G_PI);
    // gdk_cairo_set_source_rgba(cr, &fgColor);
    // cairo_fill(cr);

    cairo_set_line_width(cr, 1);
    gdk_cairo_set_source_rgba(cr, &fgColor);

    pango_layout_set_font_description(pangoLayout, state.fontDesc);
    pango_layout_set_text(pangoLayout, hexStr, strlen(hexStr));

    for(int i = 0; i < numLines; i++) {
        if(state.file) {
            fillHexBuffer(i * 0x10);
            pango_layout_set_text(pangoLayout, state.hexLineBuffer, -1);
        }
        else {
            pango_layout_set_text(pangoLayout, "", -1);
        }
        cairo_move_to(cr, 0, i * fontHeight);
        pango_cairo_show_layout(cr, pangoLayout);
    }

    g_object_unref(G_OBJECT(pangoLayout));

    return FALSE;
}

GtkWidget *buildMenu() {
    GtkWidget *menubar =     NULL;

    GtkWidget *fileMenu =    NULL;
    GtkWidget *fileMenuI =   NULL;

    GtkWidget *openMenuI =   NULL;
    GtkWidget *closeMenuI =  NULL;
    GtkWidget *fontMenuI =   NULL;
    GtkWidget *resizeMenuI = NULL;

    menubar =     gtk_menu_bar_new();
    fileMenu =    gtk_menu_new();
    fileMenuI =   gtk_menu_item_new_with_label("File");

    openMenuI =   gtk_menu_item_new_with_label("Open");
    closeMenuI =  gtk_menu_item_new_with_label("Close");
    fontMenuI =   gtk_menu_item_new_with_label("Font");
    resizeMenuI = gtk_menu_item_new_with_label("Resize");

    g_signal_connect(G_OBJECT(openMenuI),   "activate", G_CALLBACK(openMenuAction),   NULL);
    g_signal_connect(G_OBJECT(closeMenuI),  "activate", G_CALLBACK(closeCurrentFile), NULL);
    g_signal_connect(G_OBJECT(fontMenuI),   "activate", G_CALLBACK(fontMenuAction),   NULL);
    g_signal_connect(G_OBJECT(resizeMenuI), "activate", G_CALLBACK(resizeMenuAction), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMenuI);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenuI), fileMenu);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), closeMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fontMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), resizeMenuI);

    return menubar;
}

int main(int argc, char **argv) {
    GtkWidget *menubar = NULL;

    GtkWidget *vbox = NULL;

    GtkWidget *drawingArea = NULL;
    GtkStyleContext *areaStyleContext = NULL;

    PangoFontDescription *defaultFontDesc = NULL;

    gtk_init(&argc, &argv);

    state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state.window), "JAFHE");
    gtk_window_set_default_size(GTK_WINDOW(state.window), 600, 400);
    gtk_window_set_icon_from_file(GTK_WINDOW(state.window), "jafhe.svg", NULL); // Does this even do anything?
    g_signal_connect(G_OBJECT(state.window), "destroy", G_CALLBACK(shutdownAndCleanup), NULL);

    defaultFontDesc = pango_font_description_from_string("Monospace Normal 12");
    updateFont(defaultFontDesc);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), vbox);

    menubar = buildMenu();

    state.fileWidgetsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    drawingArea = gtk_drawing_area_new();
    areaStyleContext = gtk_widget_get_style_context(drawingArea);
    gtk_style_context_add_class(areaStyleContext, GTK_STYLE_CLASS_VIEW);
    g_signal_connect(drawingArea, "draw", G_CALLBACK(renderHexBox), NULL);

    gtk_box_pack_start(GTK_BOX(state.fileWidgetsBox), drawingArea, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), state.fileWidgetsBox, TRUE, TRUE, 0);

    gtk_widget_show_all(state.window);

    gtk_main();

    return 0;
}