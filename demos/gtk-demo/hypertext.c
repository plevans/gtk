/* Text View/Hypertext
 *
 * Usually, tags modify the appearance of text in the view, e.g. making it
 * bold or colored or underlined. But tags are not restricted to appearance.
 * They can also affect the behavior of mouse and key presses, as this demo
 * shows.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* Inserts a piece of text into the buffer, giving it the usual
 * appearance of a hyperlink in a web browser: blue and underlined.
 * Additionally, attaches some data on the tag, to make it recognizable
 * as a link.
 */
static void
insert_link (GtkTextBuffer *buffer,
             GtkTextIter   *iter,
             gchar         *text,
             gint           page)
{
  GtkTextTag *tag;

  tag = gtk_text_buffer_create_tag (buffer, NULL,
                                    "foreground", "blue",
                                    "underline", PANGO_UNDERLINE_SINGLE,
                                    NULL);
  g_object_set_data (G_OBJECT (tag), "page", GINT_TO_POINTER (page));
  gtk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

/* Fills the buffer with text and interspersed links. In any real
 * hypertext app, this method would parse a file to identify the links.
 */
static void
show_page (GtkTextBuffer *buffer,
           gint           page)
{
  GtkTextIter iter;

  gtk_text_buffer_set_text (buffer, "", 0);
  gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  if (page == 1)
    {
      gtk_text_buffer_insert (buffer, &iter, "Some text to show that simple ", -1);
      insert_link (buffer, &iter, "hyper text", 3);
      gtk_text_buffer_insert (buffer, &iter, " can easily be realized with ", -1);
      insert_link (buffer, &iter, "tags", 2);
      gtk_text_buffer_insert (buffer, &iter, ".", -1);
    }
  else if (page == 2)
    {
      gtk_text_buffer_insert (buffer, &iter,
                              "A tag is an attribute that can be applied to some range of text. "
                              "For example, a tag might be called \"bold\" and make the text inside "
                              "the tag bold. However, the tag concept is more general than that; "
                              "tags don't have to affect appearance. They can instead affect the "
                              "behavior of mouse and key presses, \"lock\" a range of text so the "
                              "user can't edit it, or countless other things.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);
    }
  else if (page == 3)
    {
      GtkTextTag *tag;

      tag = gtk_text_buffer_create_tag (buffer, NULL,
                                        "weight", PANGO_WEIGHT_BOLD,
                                        NULL);
      gtk_text_buffer_insert_with_tags (buffer, &iter, "hypertext:\n", -1, tag, NULL);
      gtk_text_buffer_insert (buffer, &iter,
                              "machine-readable text that is not sequential but is organized "
                              "so that related items of information are connected.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);
    }
}

/* Looks at all tags covering the position of iter in the text view,
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
static void
follow_if_link (GtkWidget   *text_view,
                GtkTextIter *iter)
{
  GSList *tags = NULL, *tagp = NULL;

  tags = gtk_text_iter_get_tags (iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      GtkTextTag *tag = tagp->data;
      gint page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page"));

      if (page != 0)
        {
          show_page (gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view)), page);
          break;
        }
    }

  if (tags)
    g_slist_free (tags);
}

/* Links can be activated by pressing Enter.
 */
static gboolean
key_press_event (GtkWidget *text_view,
                 GdkEventKey *event)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;
  guint keyval;

  gdk_event_get_keyval ((GdkEvent *)event, &keyval);

  switch (keyval)
    {
      case GDK_KEY_Return:
      case GDK_KEY_KP_Enter:
        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
        gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                          gtk_text_buffer_get_insert (buffer));
        follow_if_link (text_view, &iter);
        break;

      default:
        break;
    }

  return FALSE;
}

/* Links can also be activated by clicking or tapping.
 */
static gboolean
event_after (GtkWidget *text_view,
             GdkEvent  *ev)
{
  GtkTextIter start, end, iter;
  GtkTextBuffer *buffer;
  gdouble ex, ey;
  int x, y;
  GdkEventType type;

  type = gdk_event_get_event_type (ev);

  if (type == GDK_BUTTON_RELEASE)
    {
      guint button;

      gdk_event_get_button (ev, &button);
      if (button != GDK_BUTTON_PRIMARY)
        return FALSE;
    }
  else if (type == GDK_TOUCH_END)
    {
    }
  else
    return FALSE;

  gdk_event_get_coords (ev, &ex, &ey);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  /* we shouldn't follow a link if the user has selected something */
  gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))
    return FALSE;

  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
                                         GTK_TEXT_WINDOW_WIDGET,
                                         ex, ey, &x, &y);

  if (gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (text_view), &iter, x, y))
    follow_if_link (text_view, &iter);

  return TRUE;
}

static gboolean hovering_over_link = FALSE;

/* Looks at all tags covering the position (x, y) in the text view,
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
static void
set_cursor_if_appropriate (GtkTextView    *text_view,
                           gint            x,
                           gint            y)
{
  GSList *tags = NULL, *tagp = NULL;
  GtkTextIter iter;
  gboolean hovering = FALSE;

  if (gtk_text_view_get_iter_at_location (text_view, &iter, x, y))
    {
      tags = gtk_text_iter_get_tags (&iter);
      for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
        {
          GtkTextTag *tag = tagp->data;
          gint page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page"));

          if (page != 0)
            {
              hovering = TRUE;
              break;
            }
        }
    }

  if (hovering != hovering_over_link)
    {
      hovering_over_link = hovering;

      if (hovering_over_link)
        gtk_widget_set_cursor_from_name (GTK_WIDGET (text_view), "pointer");
      else
        gtk_widget_set_cursor_from_name (GTK_WIDGET (text_view), "text");
    }

  if (tags)
    g_slist_free (tags);
}

/* Update the cursor image if the pointer moved.
 */
static gboolean
motion_notify_event (GtkWidget      *text_view,
                     GdkEventMotion *event)
{
  gdouble ex, ey;
  gint x, y;

  gdk_event_get_coords ((GdkEvent *)event, &ex, &ey);
  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
                                         GTK_TEXT_WINDOW_WIDGET,
                                         ex, ey, &x, &y);

  set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), x, y);

  return FALSE;
}

GtkWidget *
do_hypertext (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *view;
      GtkWidget *sw;
      GtkTextBuffer *buffer;

      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title (GTK_WINDOW (window), "Hypertext");
      gtk_window_set_display (GTK_WINDOW (window),
                              gtk_widget_get_display (do_widget));
      gtk_window_set_default_size (GTK_WINDOW (window), 450, 450);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed), &window);

      view = gtk_text_view_new ();
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
      gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 20);
      gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 20);
      g_signal_connect (view, "key-press-event",
                        G_CALLBACK (key_press_event), NULL);
      g_signal_connect (view, "event",
                        G_CALLBACK (event_after), NULL);
      g_signal_connect (view, "motion-notify-event",
                        G_CALLBACK (motion_notify_event), NULL);

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

      sw = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (window), sw);
      gtk_container_add (GTK_CONTAINER (sw), view);

      show_page (buffer, 1);
    }

  if (!gtk_widget_get_visible (window))
    gtk_widget_show (window);
  else
    gtk_widget_destroy (window);

  return window;
}
