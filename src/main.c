#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <gtk/gtk.h>

#define REG_LINE_LEN 63

typedef uint8_t byte;

struct _Color {
    union {
        uint RGBA;
        struct {
            byte alpha;
            byte blue;
            byte green;
            byte red;
        };
    };
};
typedef struct _Color Color;

struct _ProgramState {
    GtkWidget *window;

    FILE *file;
    char *fileFullName;
    int fileLength;
    char *fileBuffer;
};
typedef struct _ProgramState ProgramState;

static ProgramState state = {0};

static GtkWidget *hexTextView = NULL;

void closeCurrentFile(bool titleUpdateNeeded);
void updateTitle();
PangoFontDescription *buildFont(PangoContext *context);

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

    // Todo(Adin): Clear text buffers?
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexTextView)), "", 0);
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

    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexTextView)), state.fileBuffer, -1);
}

void processAndDisplayFile() {
    // Todo(Adin): Parse file as hex and replace following with proper display
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(hexTextView)), state.fileBuffer, -1);
}

void openMenuAction(GtkMenuItem *menuItem, gpointer parentWindow) {
    GtkWidget *dialog = NULL;
    gint dialogResult = 0;

    dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(parentWindow), GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);

    dialogResult = gtk_dialog_run(GTK_DIALOG(dialog));
    if(dialogResult == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        openFile(filename);
        g_free(filename);
    }

    updateTitle();
    processAndDisplayFile();

    gtk_widget_destroy(dialog);
}

void fontMenuAction(GtkMenuItem *menuItem) {
    GtkWidget *dialog = NULL;
    gint dialogResult = 0;

    dialog = gtk_font_chooser_dialog_new("Choose Font", GTK_WINDOW(state.window));

    dialogResult = gtk_dialog_run(GTK_DIALOG(dialog));
    if(dialogResult == GTK_RESPONSE_OK) {
        // TODO(Adin): Update Font
        PangoFontDescription *fontDesc = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));
        printf("Selected Font Family: %s\n", pango_font_description_get_family(fontDesc));
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

void dumpState(GtkStateFlags flags) {
    printf("GTK_STATE_FLAG_NORMAL:        %s\n",  (flags  &  GTK_STATE_FLAG_NORMAL        ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_ACTIVE:        %s\n",  (flags  &  GTK_STATE_FLAG_ACTIVE        ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_PRELIGHT:      %s\n",  (flags  &  GTK_STATE_FLAG_PRELIGHT      ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_SELECTED:      %s\n",  (flags  &  GTK_STATE_FLAG_SELECTED      ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_INSENSITIVE:   %s\n",  (flags  &  GTK_STATE_FLAG_INSENSITIVE   ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_INCONSISTENT:  %s\n",  (flags  &  GTK_STATE_FLAG_INCONSISTENT  ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_FOCUSED:       %s\n",  (flags  &  GTK_STATE_FLAG_FOCUSED       ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_BACKDROP:      %s\n",  (flags  &  GTK_STATE_FLAG_BACKDROP      ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_DIR_LTR:       %s\n",  (flags  &  GTK_STATE_FLAG_DIR_LTR       ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_DIR_RTL:       %s\n",  (flags  &  GTK_STATE_FLAG_DIR_RTL       ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_LINK:          %s\n",  (flags  &  GTK_STATE_FLAG_LINK          ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_VISITED:       %s\n",  (flags  &  GTK_STATE_FLAG_VISITED       ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_CHECKED:       %s\n",  (flags  &  GTK_STATE_FLAG_CHECKED       ?  "true"  :  "false"));
    printf("GTK_STATE_FLAG_DROP_ACTIVE:   %s\n",  (flags  &  GTK_STATE_FLAG_DROP_ACTIVE   ?  "true"  :  "false"));
}

double adjustRange(double value, double oldmin, double oldmax, double newmin, double newmax) {
    return ((value - oldmin) / (oldmax - oldmin)) * (newmax - newmin) + newmin;
}

double normalizeColor(double color) {
    return adjustRange(color, 0, 255, 0, 1);
}

double denormalizeColor(double color) {
    return adjustRange(color, 0, 1, 0, 255);
}

void dumpFontDescritption(PangoFontDescription *desc) {
    printf("Family: %s\n", pango_font_description_get_family(desc));
    printf("Size: %d\n", pango_font_description_get_size(desc));

    switch(pango_font_description_get_style(desc)) {
        case PANGO_STYLE_NORMAL:
            printf("Style: PANGO_STYLE_NORMAL\n");
            break;

        case PANGO_STYLE_OBLIQUE:
            printf("Style: PANGO_STYLE_OBLIQUE\n");
            break;

        case PANGO_STYLE_ITALIC:
            printf("Style: PANGO_STYLE_ITALIC\n");
            break;
    }

    switch(pango_font_description_get_weight(desc)) {
        case PANGO_WEIGHT_THIN:
            printf("Weight: PANGO_WEIGHT_THIN\n");
            break;

        case PANGO_WEIGHT_ULTRALIGHT:
            printf("Weight: PANGO_WEIGHT_ULTRALIGHT\n");
            break;

        case PANGO_WEIGHT_LIGHT:
            printf("Weight: PANGO_WEIGHT_LIGHT\n");
            break;

        case PANGO_WEIGHT_SEMILIGHT:
            printf("Weight: PANGO_WEIGHT_SEMILIGHT\n");
            break;

        case PANGO_WEIGHT_BOOK:
            printf("Weight: PANGO_WEIGHT_BOOK\n");
            break;

        case PANGO_WEIGHT_NORMAL:
            printf("Weight: PANGO_WEIGHT_NORMAL\n");
            break;

        case PANGO_WEIGHT_MEDIUM:
            printf("Weight: PANGO_WEIGHT_MEDIUM\n");
            break;

        case PANGO_WEIGHT_SEMIBOLD:
            printf("Weight: PANGO_WEIGHT_SEMIBOLD\n");
            break;

        case PANGO_WEIGHT_BOLD:
            printf("Weight: PANGO_WEIGHT_BOLD\n");
            break;

        case PANGO_WEIGHT_ULTRABOLD:
            printf("Weight: PANGO_WEIGHT_ULTRABOLD\n");
            break;

        case PANGO_WEIGHT_HEAVY:
            printf("Weight: PANGO_WEIGHT_HEAVY\n");
            break;

        case PANGO_WEIGHT_ULTRAHEAVY:
            printf("Weight: PANGO_WEIGHT_ULTRAHEAVY\n");
            break;

        default:
            printf("Weight: %u\n", pango_font_description_get_weight(desc));
            break;
    }
}

Color gdkToColor(GdkRGBA gdkColor) {
    Color result = {0};

    result.red =   (byte) denormalizeColor(gdkColor.red);
    result.green = (byte) denormalizeColor(gdkColor.green);
    result.blue =  (byte) denormalizeColor(gdkColor.blue);
    result.alpha = (byte) denormalizeColor(gdkColor.alpha);

    return result;
} 

GdkRGBA colorToGdk(Color color) {
    GdkRGBA result = {0};

    result.red =   (byte) normalizeColor(color.red);
    result.green = (byte) normalizeColor(color.green);
    result.blue =  (byte) normalizeColor(color.blue);
    result.alpha = (byte) normalizeColor(color.alpha);

    return result;
}

gboolean colorADinosaur(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void) data;

    GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
    GtkStateFlags state = gtk_style_context_get_state(styleContext);
    PangoContext *pangoContext = gtk_widget_get_pango_context(widget);
    PangoFontDescription *fontDescription = pango_font_description_from_string("Monospace Normal 12");
    PangoFontMetrics *fontMetrics = pango_context_get_metrics(pangoContext, fontDescription, NULL);    

    GdkRGBA bgColor = {0};
    GdkRGBA fgColor = {0};
    GdkRGBA pink    = {0};
    Color bgEzColor = {0};
    Color fgEzColor = {0};
    Color pinkEz    = {0};
    
    gtk_style_context_get_background_color(styleContext, state, &bgColor);
    gtk_style_context_get_color(styleContext, state, &fgColor);
    bgEzColor = gdkToColor(bgColor);
    fgEzColor = gdkToColor(fgColor);

    uint width = gtk_widget_get_allocated_width(widget);
    uint height = gtk_widget_get_allocated_height(widget);

    uint fontHeight = PANGO_PIXELS(pango_font_metrics_get_ascent(fontMetrics)) + PANGO_PIXELS(pango_font_metrics_get_descent(fontMetrics)) + 2;

    printf("Name: %s\n", gtk_widget_get_name(widget));
    printf("Width: %u Height: %u\n", width, height);
    printf("BG: 0x%08X\n", bgEzColor.RGBA);
    printf("FG: 0x%08X\n", fgEzColor.RGBA);
    printf("Font Height: %d\n", fontHeight);

    // dumpState(state);
    dumpFontDescritption(fontDescription);

    gtk_render_background(styleContext, cr, 0, 0, width, height);

    // cairo_arc(cr, width / 2.0, height / 2.0, MIN(width, height) / 2.0, 0, 2 * G_PI);
    // gdk_cairo_set_source_rgba(cr, &fgColor);
    // cairo_fill(cr);

    pinkEz.RGBA = 0xFF24FEFF;
    gdk_rgba_parse(&pink, "#FF24FE");
    cairo_set_line_width(cr, 1);
    gdk_cairo_set_source_rgba(cr, &fgColor);

    uint numLines = height / fontHeight;
    if(height % fontHeight) {
        numLines++;
    }
    printf("Num Lines: %u\n", numLines);
    printf("\n");
    PangoLayout *pangoLayout = pango_layout_new(pangoContext); 
    // char *jack = "All work and no play makes Jack a dull boy";
    // pango_layout_set_text(pangoLayout, jack, strlen(jack));
    char *someHex = "0B AA FA AF 76 67 69 DB DD BB AF 76 73 00 80 90";
    pango_layout_set_font_description(pangoLayout, fontDescription);
    pango_layout_set_text(pangoLayout, someHex, strlen(someHex));

    for(int i = 0; i < numLines; i++) {
        cairo_move_to(cr, 0, i * fontHeight);
        pango_cairo_show_layout(cr, pangoLayout);
    }

    pango_font_description_free(fontDescription);

    return FALSE;
}

GtkWidget *buildMenu() {
    GtkWidget *menubar = NULL;

    GtkWidget *fileMenu = NULL;
    GtkWidget *fileMenuI = NULL;
    GtkWidget *openMenuI = NULL;
    GtkWidget *closeMenuI = NULL;
    GtkWidget *fontMenuI = NULL;

    menubar =    gtk_menu_bar_new();
    fileMenu =   gtk_menu_new();
    fileMenuI =  gtk_menu_item_new_with_label("File");
    openMenuI =  gtk_menu_item_new_with_label("Open");
    closeMenuI = gtk_menu_item_new_with_label("Close");
    fontMenuI =  gtk_menu_item_new_with_label("Font");

    g_signal_connect(G_OBJECT(openMenuI), "activate", G_CALLBACK(openMenuAction), state.window);
    g_signal_connect(G_OBJECT(closeMenuI), "activate", G_CALLBACK(closeCurrentFile), NULL);
    g_signal_connect(G_OBJECT(fontMenuI), "activate", G_CALLBACK(fontMenuAction), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMenuI);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenuI), fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), closeMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fontMenuI);

    return menubar;
}

PangoFontDescription *buildFont(PangoContext *context) {
    PangoFontDescription *requestDesc = pango_font_description_new();

    // pango_font_description_from_string("");
    pango_font_description_set_family(requestDesc, "Monospace");
    pango_font_description_set_absolute_size(requestDesc, 12);
    pango_font_description_set_variant(requestDesc, PANGO_VARIANT_NORMAL);
    pango_font_description_set_style(requestDesc, PANGO_STYLE_NORMAL);
    pango_font_description_set_weight(requestDesc, PANGO_WEIGHT_NORMAL);

    PangoFontMap *fontMap = pango_cairo_font_map_get_default();
    PangoFont *font = pango_font_map_load_font(fontMap, context, requestDesc);
    PangoFontDescription *realDesc = pango_font_describe(font);

    pango_font_description_free(requestDesc);
    
    return realDesc;
}

int main(int argc, char **argv) {
    GtkWidget *menubar = NULL;

    GtkWidget *vbox = NULL;

    GtkWidget *drawingArea = NULL;
    GtkStyleContext *areaStyleContext = NULL;
    GtkWidget *scrolledWindow = NULL;

    gtk_init(&argc, &argv);

    state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state.window), "JAFHE");
    gtk_window_set_default_size(GTK_WINDOW(state.window), 600, 400);
    gtk_window_set_icon_from_file(GTK_WINDOW(state.window), "jafhe.svg", NULL);
    g_signal_connect(G_OBJECT(state.window), "destroy", G_CALLBACK(shutdownAndCleanup), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), vbox);

    menubar = buildMenu();

    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    hexTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(hexTextView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(hexTextView), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hexTextView), FALSE);
    // gtk_container_add(GTK_CONTAINER(scrolledWindow), hexTextView);

    drawingArea = gtk_drawing_area_new();
    areaStyleContext = gtk_widget_get_style_context(drawingArea);
    gtk_style_context_add_class(areaStyleContext, GTK_STYLE_CLASS_VIEW);
    g_signal_connect(drawingArea, "draw", G_CALLBACK(colorADinosaur), NULL);
    // gtk_container_add(GTK_CONTAINER(scrolledWindow), drawingArea);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    // gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), drawingArea, TRUE, TRUE, 0);

    gtk_widget_show_all(state.window);

    gtk_main();

    return 0;
}