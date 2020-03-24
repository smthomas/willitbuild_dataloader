<?php

/**
 * @file
 * Contains the code to generate the custom drush commands.
 */

use Drupal\node\Entity\Node;
use Drupal\path_alias\Entity\PathAlias;

/**
 * Implements hook_drush_command().
 */
function willitbuild_dataloader_drush_command() {
  $items = [];

  $items['willitbuild-load'] = [
    'description' => 'Load WillItBuild JSON Data',
    'drupal dependencies' => ['willitbuild_dataloader'],
    'aliases' => ['wib:load'],
  ];

  return $items;
}

/**
 * Call back function drush_custom_drush_command_say_hello().
 *
 * The call back function name in the  following format
 *   drush_{module_name}_{item_id_for_command}()
 */
function drush_willitbuild_dataloader_willitbuild_load() {
  drush_print('Loading JSON Data');

  // Loop through JSON files.
  $directory_path = drupal_get_path('module', 'willitbuild_dataloader') . '/data';
  if ($handle = opendir($directory_path)) {

    while (FALSE !== ($entry = readdir($handle))) {
      if (stripos($entry, '.json')) {
        $json_string = file_get_contents($directory_path . '/' . $entry);
        $json = json_decode($json_string, TRUE);
        willitbuild_dataloader_load_json($json);
        drush_print('Finished loading data from ' . $entry);
      }
    }

    closedir($handle);
  }
}

/**
 * Load JSON Data.
 *
 * @param array $json
 *   JSON data to load.
 */
function willitbuild_dataloader_load_json(array $json) {
  $cnt = 0;

  // Loop through each item in the data file.
  foreach ($json as $data) {
    // Validate data.
    if (!willitbuild_dataloader_validate_data($data)) {
      drush_print('Data does not appear to be valid: ' . json_encode($data));
      continue;
    }

    // Download the image to the server and create the file entity.
    $file = willitbuild_dataloader_download_image($data['image']['url']);

    // Create the article entity.
    willitbuild_dataloader_create_article($data['articleNumber'], $data['title'], $data['content'], $file);

    $cnt++;
    if ($cnt % 10 == 0) {
      drush_print('Loaded ' . $cnt . ' articles...');
    }
  }
}

/**
 * Validate JSON Data.
 *
 * @param array $data
 *   The JSON Data to validate.
 */
function willitbuild_dataloader_validate_data(array $data) {
  $valid = TRUE;

  $valid = (empty($data['articleNumber'])) ? FALSE : $valid;
  $valid = (empty($data['title'])) ? FALSE : $valid;
  $valid = (empty($data['content'])) ? FALSE : $valid;
  $valid = (empty($data['image'] || empty($data['image']['url']))) ? FALSE : $valid;

  return $valid;
}

/**
 * Download remote image and save as file entity.
 *
 * @param string $image_url
 *   The remote image to download.
 *
 * @return object
 *   The created file entity object.
 */
function willitbuild_dataloader_download_image($image_url) {
  $image = file_get_contents($image_url);
  $filename = array_pop(explode('/', $image_url));
  return file_save_data($image, 'public://willitbuild/' . $filename);
}

/**
 * Create the article entity.
 *
 * @param int $article_number
 *   The unique article number.
 * @param string $title
 *   The title of the article.
 * @param string $body
 *   The body text of the article.
 * @param object $file
 *   The file entity.
 *
 * @return object
 *   The created article entity object.
 */
function willitbuild_dataloader_create_article($article_number, $title, $body, $file) {
  $node = Node::create([
    'type' => 'article',
    'title' => $title,
    'field_image' => [
      'target_id' => $file->id(),
      'alt' => $title,
      'title' => $title,
    ],
    'body' => [
      'value' => $body,
      'format' => 'basic_html',
    ],
  ]);

  $node->save();

  $path_alias = PathAlias::create([
    'path' => '/node/' . $node->id(),
    'alias' => str_replace(' ', '-', strtolower($title)),
  ]);
  $path_alias->save();

  return $node;
}
