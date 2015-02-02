<?php
/**
 * @file
 * Theme functions
 *
 */

/**
 * Theme tabular form
 * Arranges records in a table
 *
 * @ingroup themeable
 * @ingroup forms
 */
function theme_ucinetid_user_search($variables) {
  $variables += array('options' => array()); // add empty options if missing

  $form = $variables['form'];
  $options = $variables['options'];

  // Each record has a 'weight' that can be used to arrange it in relation to
  // other records. Drupal's tabledrag.js library allows users to control these
  // weights by dragging and dropping the records in a list -- we just need to
  // add identifying CSS classes to key elements in the table.

  $weight = 'weight'; // if there's a field with this name, make table draggable
  $draggable = '';
  $class = array();
  $rows = array();
  $header = array();
  $options += array(
    'display_header' => TRUE,  // display sticky table header
    'collapse_title' => TRUE,  // show titles only above first data row
    'collapse_description' => TRUE, // show descriptions only below last data row
  );
  $children = element_children($form);
  $last = end($children);
  foreach (element_children($form) as $key) {
    $row = array();
    // maybe should only look at first row ?
    if (empty($header) && $options['display_header']) {
      foreach (element_children($form[$key]) as $k) {
        // set header only if #title set (one table column per #title)
        if (isset($form[$key][$k]['#title'])) {
          $header[$k] = $form[$key][$k]['#title'];
        }
        if ($k == $weight && !empty($form['#parents'])) { // magic $weight column
          $class = array('draggable');
          $draggable = 'theme-draggable-' . join('-', $form['#parents']) .'-'. $k;
        }
      }
    }

    if ($draggable) {
      $form[$key][$weight]['#attributes']['class'][] = $draggable;
    }
    $output = '';
    foreach (element_children($form[$key]) as $k) {
      if ($options['collapse_title']) {
        $form[$key][$k]['#title'] = ''; // titles appear in table header
      }
      if ($options['collapse_description'] && $key != $last) {
        $form[$key][$k]['#description'] = '';
      }
      $output .= drupal_render($form[$key][$k]);
      if (isset($header[$k])) {
        $row[] = array('data' => $output, 'class' => array('table-cell-'. $k));
        $output = '';
      }
    }
    if ($output) {
      $row[] = $output;
    }

    // Add the new row to our collection of rows, and give it the 'draggable'
    // class, indicating that it should be... well, draggable.
    $rows[] = array(
      'data' => $row,
      'class' => $class,
    );
  }

  $attributes = array();
  // probably should inherit $form['#attributes']
  if (!empty($form['#attributes']['class'])) {
    $attributes['class'] = $form['#attributes']['class'];
  }
  $attributes['class'][] = 'form-table';

  if (!empty($form['#id'])) {
    $id = $attributes['id'] = $form['#id'];
  }
  elseif (!empty($form['#parents'])) {
    $id = 'theme-table-' . join('-', $form['#parents']);
    $attributes['class'][] = $id;
    $id = $attributes['id'] = drupal_html_id($id);
  }
  // Render a list of header titles, and our array of rows, into a table. Even
  // we've already rendered all of our records, we always call drupal_render()
  // on the form itself after we're done, so hidden security fields and other
  // elements (like buttons) will appear properly at the bottom of the form.
  $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
  $output .= drupal_render_children($form);

  // Now that we've built our output, tell Drupal to add the tabledrag.js library.
  // We'll pass in the ID of the table, the behavior we want it to use, and the
  // class that appears on each 'weight' form element it should be controlling.
  if ($draggable) {
    drupal_add_tabledrag($id, 'order', 'sibling', $draggable);
  }

  return $output;
}
