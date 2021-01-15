#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <gtk/gtk.h>

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
void updateTitle(bool opening);

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
        updateTitle(false);
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

    gtk_widget_destroy(dialog);

    updateTitle(true);
    processAndDisplayFile();
}

void updateTitle(bool opening) {
    char titleBuffer[39] = {0}; // 50 bytes for the file + 8 bytes for "JAFHE - " + 1 byte for terminator

    if(opening) {
        char *fullNameCopy = NULL;
        char *token = NULL;
        char *lastToken = NULL;

        fullNameCopy = malloc(strlen(state.fileFullName) + 1);
        memcpy(fullNameCopy, state.fileFullName, strlen(state.fileFullName) + 1); // + 1 for implicit null terminator
        
        token = strtok(fullNameCopy, "/");

        while(token != NULL) {
            lastToken = token;
            token = strtok(NULL, "/");
        }

        if(strlen(lastToken) > 30) {
            snprintf(titleBuffer, 39, "JAFHE - %.27s...", lastToken);
        }
        else {
            snprintf(titleBuffer, 39, "JAFHE - %s", lastToken);
        }
        gtk_window_set_title(GTK_WINDOW(state.window), titleBuffer);
    }
    else {
        gtk_window_set_title(GTK_WINDOW(state.window), "JAFHE");
    }
}

int main(int argc, char **argv) {
    GtkWidget *menubar = NULL;
    GtkWidget *fileMenu = NULL;
    GtkWidget *fileMenuI = NULL;
    GtkWidget *openMenuI = NULL;
    GtkWidget *closeMenuI = NULL;

    GtkWidget *vbox = NULL;

    GtkWidget *scrolledWindow = NULL;

    gtk_init(&argc, &argv);

    state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state.window), "JAFHE");
    gtk_window_set_default_size(GTK_WINDOW(state.window), 600, 400);
    g_signal_connect(G_OBJECT(state.window), "destroy", G_CALLBACK(shutdownAndCleanup), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), vbox);

    menubar = gtk_menu_bar_new();
    fileMenu = gtk_menu_new();
    fileMenuI = gtk_menu_item_new_with_label("File");
    openMenuI = gtk_menu_item_new_with_label("Open");
    closeMenuI = gtk_menu_item_new_with_label("Close");
    g_signal_connect(G_OBJECT(openMenuI), "activate", G_CALLBACK(openMenuAction), state.window);
    g_signal_connect(G_OBJECT(closeMenuI), "activate", G_CALLBACK(closeCurrentFile), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMenuI);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenuI), fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMenuI);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), closeMenuI);

    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    hexTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(hexTextView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(hexTextView), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hexTextView), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), hexTextView);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, TRUE, TRUE, 0);

    gtk_widget_show_all(state.window);

    gtk_main();

    return 0;
}