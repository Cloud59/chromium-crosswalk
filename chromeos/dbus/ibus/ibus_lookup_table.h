// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_IBUS_IBUS_LOOKUP_TABLE_H_
#define CHROMEOS_DBUS_IBUS_IBUS_LOOKUP_TABLE_H_

#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "chromeos/chromeos_export.h"

namespace dbus {
class MessageWriter;
class MessageReader;
}  // namespace dbus

namespace chromeos {

// The IBusLookupTable is one of IBusObjects. IBusLookupTable contains IBusTexts
// but all of them are used as plain string. The overview of each data
// structure is as follows:
//
// DATA STRUCTURE OVERVIEW:
//  variant  struct {
//   string "IBusLookupTable"
//   array [
//     dict_entry (
//       string "window_show_at_composition"
//       variant  variant  boolean false
//       ]
//     )
//   ]
//   uint32 9  // Page size
//   uint32 1  // Cursor position
//   boolean true  // Cursor visibility.
//   boolean true  // Round lookup table or not. Not used in Chrome.
//   int32 1  // Orientation
//   array [  // Array of candidate text.
//    variant struct {
//      string "IBusText"
//      array []
//      string "Candidate Text"
//      variant struct {
//       string "IBusAttrList"
//       array []
//       array []
//       }
//     }
//     ... more IBusTexts
//   ]
//   array [  // Array of label text
//    variant struct {
//      string "IBusText"
//      array []
//      string "1"
//      variant struct {
//       string "IBusAttrList"
//       array []
//       array []
//       }
//     }
//     ... more IBusTexts
//   ]
//  }
//  TODO(nona): Clean up the structure.(crbug.com/129403)
class IBusLookupTable;

// Pops a IBusLookupTable from |reader|.
// Returns false if an error occurs.
bool CHROMEOS_EXPORT PopIBusLookupTable(dbus::MessageReader* reader,
                                        IBusLookupTable* table);
// Appends a IBusLookupTable to |writer| except mozc_candidates_ in |table|.
void CHROMEOS_EXPORT AppendIBusLookupTable(const IBusLookupTable& table,
                                           dbus::MessageWriter* writer);

// An representation of IBusLookupTable object which is used in dbus
// communication with ibus-daemon.
class CHROMEOS_EXPORT IBusLookupTable {
 public:
  enum Orientation {
    HORIZONTAL = 0,
    VERTICAL = 1,
  };

  struct CandidateWindowProperty {
    CandidateWindowProperty();
    virtual ~CandidateWindowProperty();
    int page_size;
    int cursor_position;
    bool is_cursor_visible;
    bool is_vertical;
    bool show_window_at_composition;
  };

  // Represents a candidate entry. In dbus communication, each
  // field is represented as IBusText, but attributes are not used in Chrome.
  // So just simple string is sufficient in this case.
  struct Entry {
    Entry();
    virtual ~Entry();
    std::string value;
    std::string label;
    std::string annotation;
    std::string description_title;
    std::string description_body;
  };

  IBusLookupTable();
  virtual ~IBusLookupTable();

  // Returns true if the given |table| is equal to myself.
  bool IsEqual(const IBusLookupTable& table) const;

  // Copies |table| to myself.
  void CopyFrom(const IBusLookupTable& table);

  const CandidateWindowProperty& GetProperty() const {
    return *property_;
  }
  void SetProperty(const CandidateWindowProperty& property) {
    *property_ = property;
  }

  // Returns the number of candidates in one page.
  uint32 page_size() const { return property_->page_size; }
  void set_page_size(uint32 page_size) { property_->page_size = page_size; }

  // Returns the cursor index of the currently selected candidate.
  uint32 cursor_position() const { return property_->cursor_position; }
  void set_cursor_position(uint32 cursor_position) {
    property_->cursor_position = cursor_position;
  }

  // Returns true if the cursor is visible.
  bool is_cursor_visible() const { return property_->is_cursor_visible; }
  void set_is_cursor_visible(bool is_cursor_visible) {
    property_->is_cursor_visible = is_cursor_visible;
  }

  // Returns the orientation of lookup table.
  Orientation orientation() const {
    return property_->is_vertical ? VERTICAL : HORIZONTAL;
  }
  void set_orientation(Orientation orientation) {
    property_->is_vertical = (orientation == VERTICAL);
  }

  const std::vector<Entry>& candidates() const { return candidates_; }
  std::vector<Entry>* mutable_candidates() { return &candidates_; }

  bool show_window_at_composition() const {
    return property_->show_window_at_composition;
  }
  void set_show_window_at_composition(bool show_window_at_composition) {
    property_->show_window_at_composition = show_window_at_composition;
  }

 private:
  scoped_ptr<CandidateWindowProperty> property_;
  std::vector<Entry> candidates_;

  DISALLOW_COPY_AND_ASSIGN(IBusLookupTable);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_IBUS_IBUS_LOOKUP_TABLE_H_
