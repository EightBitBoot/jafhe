#include <stdint.h>

#include <gtk/gtk.h>

typedef unsigned int uint;
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

double adjustRange(double value, double oldmin, double oldmax, double newmin, double newmax) {
    return ((value - oldmin) / (oldmax - oldmin)) * (newmax - newmin) + newmin;
}

double normalizeColor(double color) {
    return adjustRange(color, 0, 255, 0, 1);
}

double denormalizeColor(double color) {
    return adjustRange(color, 0, 1, 0, 255);
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
