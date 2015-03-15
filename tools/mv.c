#include "tools.h"

static gboolean opt_force;

static GOptionEntry entries[] =
{
  //{ "force",          'f',   0, G_OPTION_ARG_NONE,    &opt_force,         "Overwrite files",                   NULL },
  { NULL }
};

int main(int ac, char* av[])
{
  gc_error_free GError *local_err = NULL;
  mega_session* s;

  tool_init(&ac, &av, "- move files on the remote filesystem at mega.co.nz", entries);

  if (ac < 3)
  {
    g_printerr("ERROR: You must specify both source path(s) and destination path");
    return 1;
  }

  s = tool_start_session();
  if (!s)
    return 1;

  gboolean rename = FALSE;
  gc_free gchar* dest = tool_convert_filename(av[ac - 1], FALSE);

  // check destination path
  mega_node* destination = mega_session_stat(s, dest);
  if (destination)
  {
    if (destination->type == MEGA_NODE_FILE)
    {
      gc_free gchar* path = mega_node_get_path_dup(destination);
      g_printerr("Destination file already exists: %s", path);
      goto err;
    }

    if (!mega_node_is_writable(s, destination) || destination->type == MEGA_NODE_NETWORK || destination->type == MEGA_NODE_CONTACT)
    {
      gc_free gchar* path = mega_node_get_path_dup(destination);
      g_printerr("You can't move files into: %s", path);
      goto err;
    }
  }
  else
  {
    rename = TRUE;

    gc_free gchar* parent_path = g_path_get_dirname(dest);
    destination = mega_session_stat(s, parent_path);

    if (!destination)
    {
      g_printerr("Destination directory doesn't exist: %s", parent_path);
      goto err;
    }

    if (destination->type == MEGA_NODE_FILE)
    {
      gc_free gchar* path = mega_node_get_path_dup(destination);
      g_printerr("Destination is not directory: %s", path);
      goto err;
    }

    if (!mega_node_is_writable(s, destination) || destination->type == MEGA_NODE_NETWORK || destination->type == MEGA_NODE_CONTACT)
    {
      gc_free gchar* path = mega_node_get_path_dup(destination);
      g_printerr("You can't move files into: %s", path);
      goto err;
    }
  }

  if (rename && ac > 3)
  {
    g_printerr("You can't use multiple source paths when renaming file or directory");
    goto err;
  }

  // enumerate source paths
  gint i;
  for (i = 1; i < ac - 1; i++)
  {
    gc_free gchar* path = tool_convert_filename(av[i], FALSE);
    mega_node* n = mega_session_stat(s, path);

    if (!n)
    {
      g_printerr("Source file doesn't exists: %s", path);
      goto err;
    }

    if (n->type != MEGA_NODE_FILE && n->type != MEGA_NODE_FOLDER)
    {
      g_printerr("Source is not movable: %s", path);
      goto err;
    }

    // check destination
    gc_free gchar* n_path = mega_node_get_path_dup(n);
    gc_free gchar* destination_path = mega_node_get_path_dup(destination);
    gc_free gchar* basename = g_path_get_basename(n_path);
    gc_free gchar* tmp = g_strconcat(destination_path, "/", basename, NULL);

    // check destination path
    mega_node* dn = mega_session_stat(s, tmp);
    if (dn)
    {
      gc_free gchar* dn_path = mega_node_get_path_dup(dn);
      g_printerr("Destination file already exists: %s", dn_path);
      goto err;
    }

    // perform move
    //if (!mega_session_mkdir(s, path, &local_err))
    //{
      //g_printerr("ERROR: Can't create directory %s: %s\n", path, local_err->message);
      //g_clear_error(&local_err);
    //}

    g_print("mv %s %s/%s\n", n_path, destination_path, tmp);
  }

  mega_session_save(s, NULL);

  tool_fini(s);
  return 0;

err:
  tool_fini(s);
  return 1;
}
