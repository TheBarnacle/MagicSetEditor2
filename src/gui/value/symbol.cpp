//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/value/symbol.hpp>
#include <gui/symbol/window.hpp>
#include <data/action/value.hpp>
#include <gui/util.hpp>

// ----------------------------------------------------------------------------- : SymbolValueEditor

IMPLEMENT_VALUE_EDITOR(Symbol)
  , button_down(-2)
{
  button_images[0] = Bitmap(load_resource_image(_("edit_symbol")));
}

void SymbolValueEditor::draw(RotatedDC& dc) {
  SymbolValueViewer::draw(dc);
  // draw helper text if there are no symbols
  if (symbols.empty()) {
    dc.SetFont(wxFont(10,wxFONTFAMILY_SWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(*wxBLACK);
    RealSize text_size = dc.GetTextExtent(_("double click to edit symbol"));
    dc.DrawText(_("double click to edit symbol"), align_in_rect(ALIGN_MIDDLE_CENTER, text_size, dc.getInternalRect()));
  }
  if (nativeLook()) {
    // draw editor buttons
    dc.SetFont(*wxNORMAL_FONT);
    drawButton(dc, 0, _BUTTON_("edit symbol"));
    //drawButton(dc, 1, _BUTTON_("symbol gallery"));
  }
}
void SymbolValueEditor::drawButton(RotatedDC& dc, int button, const String& text) {
  bool down = button == button_down;
  double height = bounding_box.height;
  double width  = bounding_box.height + 2;
  double x = bounding_box.width - width - (width + 1) * button;
  double y = 0;
  // draw button
  draw_button(&editor(), dc.getDC(), dc.trRectToBB(RealRect(x,y,width,height)), false, down, true);
  // draw text
  RealSize text_size = dc.GetTextExtent(text);
  dc.DrawText(text, align_in_rect((Alignment)(ALIGN_BOTTOM | ALIGN_CENTER), text_size, RealRect(x, y, width,height*0.9)));
  // draw image
  const Bitmap& bmp = button_images[button];
  RealSize image_size(bmp.GetWidth(), bmp.GetHeight());
  dc.DrawBitmap(bmp, align_in_rect(ALIGN_MIDDLE_CENTER, image_size, RealRect(x,y,width,height * 0.8)));
}

int SymbolValueEditor::findButton(const RealPoint& pos) {
  if (pos.y < 0 || pos.y >= bounding_box.height) return -1;
  int button = (int)floor( (bounding_box.width - pos.x) / (bounding_box.height + 3) );
  if (button >= 0 && button <= 1) return button;
  return -1;
}

bool SymbolValueEditor::onLeftDown(const RealPoint& pos, wxMouseEvent&) {
  if (!nativeLook()) return false;
  int button = findButton(pos);
  if (button != button_down) {
    button_down = button;
    parent.redraw(*this);
  }
  return true;
}
bool SymbolValueEditor::onMotion(const RealPoint& pos, wxMouseEvent& ev) {
  if (button_down != -2) {
    int button = findButton(pos);
    if (button != button_down) {
      button_down = button;
      parent.redraw(*this);
    }
  }
  return true;
}

bool SymbolValueEditor::onLeftUp(const RealPoint& pos, wxMouseEvent&) {
  if (!nativeLook()) return false;
  if (button_down == 0) {
    // edit
    button_down = -2;
    parent.redraw(*this);
    editSymbol();
    return true;
  } else if (button_down == 1) {
    // gallery
    button_down = -2;
    parent.redraw(*this);
    // TODO
    return true;
  } else {
    button_down = -2;
    return false;
  }
}

bool SymbolValueEditor::onLeftDClick(const RealPoint& pos, wxMouseEvent&) {
  // Use SetWindow as parent? Maybe not, the symbol editor will stay open when mainwindow closes
  editSymbol();
  return true;
}

void SymbolValueEditor::determineSize(bool) {
  if (style().height == 0) style().height = 50;
  bounding_box.height = 50;
}


void SymbolValueEditor::editSymbol() {
  SymbolWindow* wnd = new SymbolWindow(nullptr, getActionPerformer());
  wnd->Show();
}

unique_ptr<ValueActionPerformer> SymbolValueEditor::getActionPerformer() {
  return make_unique<ValueActionPerformer>(valueP(), editor().getCard(), editor().getSetForActions());
}
