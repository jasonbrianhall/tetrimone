// ============================================================================
// wx_dialog_helpers.cpp - wxWidgets dialogs for Tetrimone
// ============================================================================

#include "wx_dialog_helpers.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/radiobox.h>
#include <wx/scrolwin.h>
#include <wx/slider.h>
#include <wx/statline.h>
#include <wx/filedlg.h>

#include <algorithm>
#include <cstdio>

namespace WXHelpers {

namespace {

wxString U(const std::string& s) { return wxString::FromUTF8(s.c_str()); }

// wx wraps at a pixel width; keep the text comfortably inside the dialog.
wxStaticText* makeLabel(wxWindow* parent, const TextConfig& config, int wrapWidth) {
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_CENTRE_HORIZONTAL);
    if (config.isMarkup) {
        if (!label->SetLabelMarkup(U(config.markup))) {
            // Fall back to the text with tags stripped if the markup is rejected
            label->SetLabelText(U(stripMarkup(config.markup)));
        }
    } else {
        label->SetLabelText(U(config.content));
        if (wrapWidth > 0) label->Wrap(wrapWidth);
    }
    return label;
}

void addLabel(wxWindow* parent, wxSizer* sizer, const TextConfig& config, int wrapWidth) {
    int top = parent->FromDIP(static_cast<int>(config.marginTop));
    int bottom = parent->FromDIP(static_cast<int>(config.marginBottom));
    if (top > 0) sizer->AddSpacer(top);
    sizer->Add(makeLabel(parent, config, wrapWidth), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, parent->FromDIP(2));
    if (bottom > 0) sizer->AddSpacer(bottom);
}

// Size a dialog to at least the requested GTK default size, but never larger
// than 90% of the display it is on.
void sizeDialog(wxDialog* dialog, int width, int height) {
    dialog->Layout();
    dialog->Fit();
    wxSize best = dialog->GetSize();
    wxSize wanted(std::max(best.x, dialog->FromDIP(width)),
                  std::max(best.y, dialog->FromDIP(height)));

    wxRect display = wxGetClientDisplayRect();
    wanted.x = std::min(wanted.x, display.width * 9 / 10);
    wanted.y = std::min(wanted.y, display.height * 9 / 10);
    dialog->SetSize(wanted);
    dialog->CentreOnParent();
}

wxString wildcardFromFilter(const std::string& filter, const std::string& description) {
    // "*.png;*.jpg" -> "Description (*.png;*.jpg)|*.png;*.jpg"
    wxString pattern = U(filter);
    return U(description) + " (" + pattern + ")|" + pattern + "|All files (*.*)|*.*";
}

// A horizontal slider with a live numeric readout on the right, like a
// GtkScale with value-pos RIGHT.
class ValueSlider : public wxPanel {
public:
    ValueSlider(wxWindow* parent, double minValue, double maxValue, double step,
                double value, int digits, const std::string& readoutFormat = std::string())
        : wxPanel(parent), min_(minValue), step_(step > 0 ? step : 1.0), digits_(digits),
          format_(readoutFormat) {
        int steps = static_cast<int>((maxValue - minValue) / step_ + 0.5);
        int pos = static_cast<int>((value - minValue) / step_ + 0.5);
        slider_ = new wxSlider(this, wxID_ANY, std::clamp(pos, 0, steps), 0, steps,
                               wxDefaultPosition, wxSize(FromDIP(220), -1));
        readout_ = new wxStaticText(this, wxID_ANY, format(maxValue),
                                    wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        // Reserve room for the widest value so the slider doesn't jump around
        wxSize widest = readout_->GetBestSize();
        readout_->SetLabel(format(minValue));
        widest.IncTo(readout_->GetBestSize());
        readout_->SetMinSize(widest);
        readout_->SetLabel(format(getValue()));

        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(slider_, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(readout_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
        SetSizer(row);

        slider_->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
            readout_->SetLabel(format(getValue()));
            if (onChange_) onChange_(getValue());
        });
    }

    double getValue() const { return min_ + slider_->GetValue() * step_; }
    void onChange(std::function<void(double)> fn) { onChange_ = std::move(fn); }

private:
    wxString format(double v) const {
        if (!format_.empty()) return wxString::Format(wxString::FromUTF8(format_.c_str()), v);
        return wxString::Format("%.*f", digits_, v);
    }

    double min_, step_;
    int digits_;
    std::string format_;
    wxSlider* slider_;
    wxStaticText* readout_;
    std::function<void(double)> onChange_;
};

// Min/max captions under a slider ("Mute ... Max")
wxSizer* rangeCaptions(wxWindow* parent, const std::string& left, const std::string& right) {
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(parent, wxID_ANY, U(left)), 0);
    row->AddStretchSpacer();
    row->Add(new wxStaticText(parent, wxID_ANY, U(right)), 0);
    return row;
}

} // namespace

std::string stripMarkup(const std::string& markup) {
    std::string out;
    out.reserve(markup.size());
    bool inTag = false;
    for (size_t i = 0; i < markup.size(); ++i) {
        char c = markup[i];
        if (inTag) {
            if (c == '>') inTag = false;
            continue;
        }
        if (c == '<') {
            inTag = true;
            continue;
        }
        if (c == '&') {
            static const std::pair<const char*, char> entities[] = {
                {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}};
            bool matched = false;
            for (const auto& e : entities) {
                size_t len = std::char_traits<char>::length(e.first);
                if (markup.compare(i, len, e.first) == 0) {
                    out += e.second;
                    i += len - 1;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
        }
        out += c;
    }
    return out;
}

std::string convertMnemonic(const std::string& gtkLabel) {
    std::string out;
    out.reserve(gtkLabel.size());
    for (size_t i = 0; i < gtkLabel.size(); ++i) {
        char c = gtkLabel[i];
        if (c == '&') {
            out += "&&";
        } else if (c == '_') {
            if (i + 1 < gtkLabel.size() && gtkLabel[i + 1] == '_') {
                out += '_';
                ++i;
            } else {
                out += '&';
            }
        } else {
            out += c;
        }
    }
    return out;
}

// ============================================================================
// File dialogs
// ============================================================================

std::string WXFileDialog::openFile(const std::string& title,
                                   const std::string& filter,
                                   const std::string& filterDescription) {
    wxFileDialog dialog(parentWindow, U(title), wxEmptyString, wxEmptyString,
                        wildcardFromFilter(filter, filterDescription),
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK) return std::string();
    return std::string(dialog.GetPath().utf8_str());
}

std::vector<std::string> WXFileDialog::openFiles(const std::string& title,
                                                 const std::string& filter,
                                                 const std::string& filterDescription) {
    std::vector<std::string> result;
    wxFileDialog dialog(parentWindow, U(title), wxEmptyString, wxEmptyString,
                        wildcardFromFilter(filter, filterDescription),
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (dialog.ShowModal() != wxID_OK) return result;
    wxArrayString paths;
    dialog.GetPaths(paths);
    for (const wxString& p : paths) result.emplace_back(p.utf8_str());
    return result;
}

void WXFileDialog::showError(const std::string& title, const std::string& message) {
    WXHelpers::showError(parentWindow, message, title);
}

// ============================================================================
// Message helpers
// ============================================================================

bool askYesNo(wxWindow* parent, const std::string& message, const std::string& title) {
    wxMessageDialog dialog(parent, U(message), U(title), wxYES_NO | wxICON_QUESTION);
    return dialog.ShowModal() == wxID_YES;
}

void showInfo(wxWindow* parent, const std::string& message, const std::string& title) {
    wxMessageDialog dialog(parent, U(message), U(title), wxOK | wxICON_INFORMATION);
    dialog.ShowModal();
}

void showError(wxWindow* parent, const std::string& message, const std::string& title) {
    wxMessageDialog dialog(parent, U(message), U(title), wxOK | wxICON_ERROR);
    dialog.ShowModal();
}

// ============================================================================
// Generic text dialog (about, propaganda, freedom report)
// ============================================================================

int createAndRunDialog(wxWindow* parent,
                       const DialogConfig& dialogConfig,
                       const std::vector<TextConfig>& textElements,
                       const RadioGroupConfig* radioConfig,
                       const std::vector<TextConfig>& footerElements) {
    wxDialog dialog(parent, wxID_ANY, U(dialogConfig.title), wxDefaultPosition,
                    wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);

    // Scrollable body so long texts still fit on small screens
    wxScrolledWindow* body = new wxScrolledWindow(&dialog, wxID_ANY);
    body->SetScrollRate(0, dialog.FromDIP(10));
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    int wrap = dialog.FromDIP(std::max(200, dialogConfig.width - 60));

    for (const auto& text : textElements) addLabel(body, vbox, text, wrap);

    if (radioConfig) {
        wxArrayString choices;
        for (const auto& option : radioConfig->options) choices.Add(U(option));
        wxRadioBox* radio = new wxRadioBox(body, wxID_ANY, U(radioConfig->frameTitle),
                                           wxDefaultPosition, wxDefaultSize, choices,
                                           1, wxRA_SPECIFY_COLS);
        if (radioConfig->defaultSelectedIndex >= 0 &&
            radioConfig->defaultSelectedIndex < static_cast<int>(choices.size())) {
            radio->SetSelection(radioConfig->defaultSelectedIndex);
        }
        vbox->Add(radio, 0, wxEXPAND | wxALL, dialog.FromDIP(5));
    }

    for (const auto& text : footerElements) addLabel(body, vbox, text, wrap);

    body->SetSizer(vbox);
    body->FitInside();
    // Let the scrolled area ask for its natural size up to the dialog size
    wxSize natural = vbox->GetMinSize();
    body->SetMinSize(wxSize(natural.x,
                            std::min(natural.y, dialog.FromDIP(std::max(200, dialogConfig.height - 80)))));

    outer->Add(body, 1, wxEXPAND | wxALL, dialog.FromDIP(15));

    wxButton* ok = new wxButton(&dialog, wxID_OK, U(convertMnemonic(dialogConfig.acceptButtonLabel)));
    ok->SetDefault();
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    buttons->Add(ok, 0);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(15));

    dialog.SetSizer(outer);
    sizeDialog(&dialog, dialogConfig.width, dialogConfig.height);
    ok->SetFocus();

    return dialog.ShowModal() == wxID_OK ? RESPONSE_OK : RESPONSE_CANCEL;
}

// ============================================================================
// Scrolled text dialog (instructions)
// ============================================================================

void createAndRunScrolledTextDialog(wxWindow* parent,
                                    const DialogConfig& dialogConfig,
                                    const ScrolledTextConfig& textConfig) {
    wxDialog dialog(parent, wxID_ANY, U(dialogConfig.title), wxDefaultPosition,
                    wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);

    wxTextCtrl* text = new wxTextCtrl(&dialog, wxID_ANY, U(textConfig.content),
                                      wxDefaultPosition,
                                      dialog.FromDIP(wxSize(textConfig.width, textConfig.height)),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_WORDWRAP);

    if (!textConfig.fontDescription.empty()) {
        // Parse a Pango-ish description: "<family words> <size>"
        std::string desc = textConfig.fontDescription;
        int pointSize = 10;
        size_t lastSpace = desc.find_last_of(' ');
        if (lastSpace != std::string::npos) {
            int parsed = std::atoi(desc.c_str() + lastSpace + 1);
            if (parsed > 0) {
                pointSize = parsed;
                desc = desc.substr(0, lastSpace);
            }
        }
        wxString family = U(desc).Lower();
        wxFontFamily ff = wxFONTFAMILY_DEFAULT;
        if (family.Contains("mono")) ff = wxFONTFAMILY_TELETYPE;
        else if (family.Contains("serif") && !family.Contains("sans")) ff = wxFONTFAMILY_ROMAN;
        else if (family.Contains("sans")) ff = wxFONTFAMILY_SWISS;
        text->SetFont(wxFontInfo(pointSize).Family(ff));
    }
    text->SetInsertionPoint(0);

    outer->Add(text, 1, wxEXPAND | wxALL, dialog.FromDIP(15));

    wxButton* ok = new wxButton(&dialog, wxID_OK, U(convertMnemonic(dialogConfig.acceptButtonLabel)));
    ok->SetDefault();
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    buttons->Add(ok, 0);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(15));

    dialog.SetSizer(outer);
    sizeDialog(&dialog, dialogConfig.width, dialogConfig.height);
    ok->SetFocus();
    dialog.ShowModal();
}

// ============================================================================
// High score entry
// ============================================================================

std::string createScoreEntryDialog(wxWindow* parent, const ScoreEntryConfig& config) {
    wxDialog dialog(parent, wxID_ANY, U(config.title));
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    int pad = dialog.FromDIP(5);

    wxStaticText* scoreLabel = new wxStaticText(&dialog, wxID_ANY, wxEmptyString);
    scoreLabel->SetLabelMarkup(wxString::Format("Score: <b>%d</b>", config.score));
    vbox->Add(scoreLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, pad);
    vbox->Add(new wxStaticText(&dialog, wxID_ANY, "Difficulty: " + U(config.difficulty)),
              0, wxALIGN_CENTER_HORIZONTAL | wxALL, pad);
    vbox->Add(new wxStaticText(&dialog, wxID_ANY, U(config.gridSize)),
              0, wxALIGN_CENTER_HORIZONTAL | wxALL, pad);
    vbox->Add(new wxStaticText(&dialog, wxID_ANY, U(config.junkInfo)),
              0, wxALIGN_CENTER_HORIZONTAL | wxALL, pad);
    vbox->Add(new wxStaticLine(&dialog), 0, wxEXPAND | wxALL, pad);
    vbox->Add(new wxStaticText(&dialog, wxID_ANY, "Enter your name:"), 0, wxALIGN_CENTER_HORIZONTAL);

    wxTextCtrl* entry = new wxTextCtrl(&dialog, wxID_ANY);
    entry->SetHint("Anonymous");
    entry->SetMaxLength(40);
    vbox->Add(entry, 0, wxEXPAND | wxALL, pad);

    outer->Add(vbox, 1, wxEXPAND | wxALL, dialog.FromDIP(15));

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* submit = new wxButton(&dialog, wxID_OK, "&Submit");
    wxButton* cancel = new wxButton(&dialog, wxID_CANCEL, "&Cancel");
    submit->SetDefault();
    buttons->AddStretchSpacer();
    buttons->Add(submit, 0, wxRIGHT, pad);
    buttons->Add(cancel, 0);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(15));

    dialog.SetSizer(outer);
    sizeDialog(&dialog, 400, 250);
    entry->SetFocus();

    if (dialog.ShowModal() != wxID_OK) return std::string();

    wxString name = entry->GetValue().Strip(wxString::both);
    return name.empty() ? std::string("Anonymous") : std::string(name.utf8_str());
}

// ============================================================================
// High score viewer
// ============================================================================

namespace {

class ScoreList : public wxListCtrl {
public:
    ScoreList(wxWindow* parent, std::vector<Score> scores)
        : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL),
          scores_(std::move(scores)) {
        AppendColumn("Name");
        AppendColumn("Score", wxLIST_FORMAT_RIGHT);
        AppendColumn("Difficulty");
        AppendColumn("Grid Size");
        AppendColumn("Junk Lines");
        populate();

        Bind(wxEVT_LIST_COL_CLICK, [this](wxListEvent& e) {
            int col = e.GetColumn();
            if (col < 0) return;
            ascending_ = (col == sortColumn_) ? !ascending_ : (col != 1);  // scores default high-first
            sortColumn_ = col;
            sortRows();
            populate();
        });
        Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { e.Skip(); CallAfter([this] { fitColumns(); }); });
    }

private:
    static wxString gridText(const Score& s) { return wxString::Format("%d x %d", s.width, s.height); }
    static wxString junkText(const Score& s) {
        return wxString::Format("Init: %d%%, Level: %d", s.initialJunkPercent, s.junkLinesPerLevel);
    }

    void sortRows() {
        auto key = [this](const Score& a, const Score& b) -> bool {
            switch (sortColumn_) {
                case 0: return wxString::FromUTF8(a.name.c_str()).CmpNoCase(wxString::FromUTF8(b.name.c_str())) < 0;
                case 1: return a.score < b.score;
                case 2: return a.difficulty < b.difficulty;
                case 3: return a.width * 100 + a.height < b.width * 100 + b.height;
                case 4: return a.initialJunkPercent * 100 + a.junkLinesPerLevel <
                               b.initialJunkPercent * 100 + b.junkLinesPerLevel;
                default: return false;
            }
        };
        std::stable_sort(scores_.begin(), scores_.end(), [&](const Score& a, const Score& b) {
            return ascending_ ? key(a, b) : key(b, a);
        });
    }

    void populate() {
        Freeze();
        DeleteAllItems();
        long row = 0;
        for (const Score& s : scores_) {
            long idx = InsertItem(row++, wxString::FromUTF8(s.name.c_str()));
            SetItem(idx, 1, wxString::Format("%d", s.score));
            SetItem(idx, 2, wxString::FromUTF8(s.difficulty.c_str()));
            SetItem(idx, 3, gridText(s));
            SetItem(idx, 4, junkText(s));
        }
        Thaw();
        fitColumns();
    }

    void fitColumns() {
        int total = GetClientSize().x;
        if (total <= 0) return;
        // Name gets the slack, the rest share evenly
        int other = total / 6;
        for (int c = 1; c < 5; ++c) SetColumnWidth(c, other);
        SetColumnWidth(0, std::max(other, total - other * 4 - 2));
    }

    std::vector<Score> scores_;
    int sortColumn_ = -1;
    bool ascending_ = true;
};

} // namespace

void createScoreTabulatorDialog(wxWindow* parent, const ScoreTabulatorConfig& config) {
    wxDialog dialog(parent, wxID_ANY, U(config.title), wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);

    wxNotebook* notebook = new wxNotebook(&dialog, wxID_ANY);
    for (const auto& tab : config.tabs) {
        ScoreList* list = new ScoreList(notebook, tab.scores);
        notebook->AddPage(list, wxString::Format("%s (%zu)", U(tab.tabName), tab.scores.size()));
    }
    outer->Add(notebook, 1, wxEXPAND | wxALL, dialog.FromDIP(10));

    wxButton* close = new wxButton(&dialog, wxID_OK, "&Close");
    close->SetDefault();
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    buttons->Add(close, 0);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(10));

    dialog.SetSizer(outer);
    dialog.SetEscapeId(wxID_OK);
    sizeDialog(&dialog, config.width, config.height);
    dialog.ShowModal();
}

// ============================================================================
// Background opacity
// ============================================================================

void createOpacitySliderDialog(wxWindow* parent,
                               const OpacitySliderConfig& config,
                               std::function<void(double)> onValueChanged) {
    wxDialog dialog(parent, wxID_ANY, U(config.title));
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    vbox->Add(new wxStaticText(&dialog, wxID_ANY, "Adjust background opacity:"),
              0, wxBOTTOM, dialog.FromDIP(10));

    ValueSlider* slider = new ValueSlider(&dialog, config.minValue, config.maxValue,
                                          config.stepValue, config.currentValue, 2);
    slider->onChange(onValueChanged);
    vbox->Add(slider, 0, wxEXPAND | wxBOTTOM, dialog.FromDIP(5));
    vbox->Add(rangeCaptions(&dialog, "Transparent", "Opaque"), 0, wxEXPAND);

    outer->Add(vbox, 1, wxEXPAND | wxALL, dialog.FromDIP(10));
    wxButton* ok = new wxButton(&dialog, wxID_OK, "&OK");
    ok->SetDefault();
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    buttons->Add(ok);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(10));

    dialog.SetSizer(outer);
    dialog.SetEscapeId(wxID_OK);
    sizeDialog(&dialog, config.width, config.height);
    dialog.ShowModal();
}

// ============================================================================
// Joystick mapping
// ============================================================================

void createJoystickMappingDialog(wxWindow* parent,
                                 const JoystickMappingConfig& config,
                                 JoystickApplyCallback onApply) {
    wxDialog dialog(parent, wxID_ANY, U(config.title), wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    wxScrolledWindow* body = new wxScrolledWindow(&dialog);
    body->SetScrollRate(0, dialog.FromDIP(10));
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    int pad = dialog.FromDIP(5);

    auto makeChoice = [&](const wxString& prefix, int count, int selected) {
        wxChoice* choice = new wxChoice(body, wxID_ANY);
        for (int i = 0; i < count; ++i) choice->Append(wxString::Format("%s %d", prefix, i));
        if (selected >= 0 && selected < count) choice->SetSelection(selected);
        return choice;
    };

    wxFlexGridSizer* grid = new wxFlexGridSizer(3, pad, pad * 2);
    grid->AddGrowableCol(1, 1);
    auto addRow = [&](const wxString& label, wxWindow* control, wxWindow* extra) {
        grid->Add(new wxStaticText(body, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        grid->Add(control, 1, wxEXPAND);
        if (extra) grid->Add(extra, 0, wxALIGN_CENTER_VERTICAL);
        else grid->AddSpacer(0);
    };

    wxStaticText* buttonsHeader = new wxStaticText(body, wxID_ANY, wxEmptyString);
    buttonsHeader->SetLabelMarkup("<b>Button Mappings:</b>");
    vbox->Add(buttonsHeader, 0, wxBOTTOM, pad);

    wxChoice* rotateCW  = makeChoice("Button", config.numButtons, config.rotate_cw);
    wxChoice* rotateCCW = makeChoice("Button", config.numButtons, config.rotate_ccw);
    wxChoice* hardDrop  = makeChoice("Button", config.numButtons, config.hard_drop);
    wxChoice* pause     = makeChoice("Button", config.numButtons, config.pause_button);
    addRow("Rotate Clockwise:", rotateCW, nullptr);
    addRow("Rotate Counter-CW:", rotateCCW, nullptr);
    addRow("Hard Drop:", hardDrop, nullptr);
    addRow("Pause (Start):", pause, nullptr);

    wxChoice* xAxis = makeChoice("Axis", config.numAxes, config.x_axis);
    wxChoice* yAxis = makeChoice("Axis", config.numAxes, config.y_axis);
    wxCheckBox* invertX = new wxCheckBox(body, wxID_ANY, "Invert");
    wxCheckBox* invertY = new wxCheckBox(body, wxID_ANY, "Invert");
    invertX->SetValue(config.invert_x);
    invertY->SetValue(config.invert_y);

    vbox->Add(grid, 0, wxEXPAND | wxBOTTOM, pad * 2);

    wxStaticText* axesHeader = new wxStaticText(body, wxID_ANY, wxEmptyString);
    axesHeader->SetLabelMarkup("<b>Axis Mappings:</b>");
    vbox->Add(axesHeader, 0, wxBOTTOM, pad);

    wxFlexGridSizer* axisGrid = new wxFlexGridSizer(3, pad, pad * 2);
    axisGrid->AddGrowableCol(1, 1);
    axisGrid->Add(new wxStaticText(body, wxID_ANY, "X Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    axisGrid->Add(xAxis, 1, wxEXPAND);
    axisGrid->Add(invertX, 0, wxALIGN_CENTER_VERTICAL);
    axisGrid->Add(new wxStaticText(body, wxID_ANY, "Y Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    axisGrid->Add(yAxis, 1, wxEXPAND);
    axisGrid->Add(invertY, 0, wxALIGN_CENTER_VERTICAL);
    vbox->Add(axisGrid, 0, wxEXPAND | wxBOTTOM, pad * 2);

    wxButton* apply = new wxButton(body, wxID_APPLY, "Apply Mapping");
    wxStaticText* applied = new wxStaticText(body, wxID_ANY, wxEmptyString);
    wxBoxSizer* applyRow = new wxBoxSizer(wxHORIZONTAL);
    applyRow->Add(applied, 1, wxALIGN_CENTER_VERTICAL);
    applyRow->Add(apply, 0);
    vbox->Add(applyRow, 0, wxEXPAND | wxTOP, pad);

    apply->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        if (onApply) {
            onApply(rotateCW->GetSelection(), rotateCCW->GetSelection(),
                    hardDrop->GetSelection(), pause->GetSelection(),
                    xAxis->GetSelection(), yAxis->GetSelection(),
                    invertX->GetValue(), invertY->GetValue());
        }
        applied->SetLabel("Mapping saved.");
    });

    body->SetSizer(vbox);
    body->FitInside();
    body->SetMinSize(vbox->GetMinSize());
    outer->Add(body, 1, wxEXPAND | wxALL, dialog.FromDIP(15));

    wxButton* ok = new wxButton(&dialog, wxID_OK, "&OK");
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    buttons->Add(ok);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(15));

    dialog.SetSizer(outer);
    dialog.SetEscapeId(wxID_OK);
    sizeDialog(&dialog, config.width, config.height);
    dialog.ShowModal();
}

// ============================================================================
// Game setup
// ============================================================================

void createGameSetupDialog(wxWindow* parent,
                           const GameSetupConfig& config,
                           GameSetupApplyCallback onApply) {
    wxDialog dialog(parent, wxID_ANY, U(config.title), wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    int pad = dialog.FromDIP(5);
    int wrap = dialog.FromDIP(420);

    auto section = [&](const wxString& title, ValueSlider*& slider, double minV, double maxV,
                       int value, const wxString& caption, const wxString& description) {
        wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, &dialog, title);
        wxWindow* sb = box->GetStaticBox();
        slider = new ValueSlider(sb, minV, maxV, 1.0, value, 0);
        box->Add(slider, 0, wxEXPAND | wxALL, pad);
        wxStaticText* cap = new wxStaticText(sb, wxID_ANY, caption);
        cap->Wrap(wrap);
        box->Add(cap, 0, wxLEFT | wxRIGHT | wxBOTTOM, pad);
        if (!description.empty()) {
            wxStaticText* desc = new wxStaticText(sb, wxID_ANY, description);
            desc->Wrap(wrap);
            box->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, pad);
        }
        vbox->Add(box, 0, wxEXPAND | wxBOTTOM, pad * 2);
    };

    ValueSlider* junk = nullptr;
    ValueSlider* junkPerLevel = nullptr;
    ValueSlider* level = nullptr;

    section("Initial Junk Lines", junk, 0, 50, config.junkPercentage,
            "Percentage of board to fill with junk lines (0-50%)",
            "Junk lines contain random blocks with at least 4 empty spaces per row.\n"
            "Similar colors have a higher chance of being placed adjacent to each other.");
    section("Junk Lines Per Level", junkPerLevel, 0, 5, config.junkPerLevel,
            "Number of junk lines to add when advancing to a new level (0-5)",
            "These junk lines will push up from the bottom of the board\n"
            "when you advance to a new level, increasing the challenge.");
    section("Starting Level", level, 1, 100, config.initialLevel,
            "Start at higher levels for increased difficulty and points", wxEmptyString);

    wxStaticText* warning = new wxStaticText(&dialog, wxID_ANY, wxEmptyString);
    warning->SetLabelMarkup("<span foreground='red'>Note:</span> Applying these settings will restart the current game.");
    vbox->Add(warning, 0, wxTOP | wxBOTTOM, pad * 2);

    outer->Add(vbox, 1, wxEXPAND | wxALL, dialog.FromDIP(10));

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* apply = new wxButton(&dialog, wxID_APPLY, "&Apply");
    wxButton* cancel = new wxButton(&dialog, wxID_CANCEL, "&Cancel");
    apply->SetDefault();
    apply->Bind(wxEVT_BUTTON, [&dialog](wxCommandEvent&) { dialog.EndModal(wxID_APPLY); });
    buttons->AddStretchSpacer();
    buttons->Add(apply, 0, wxRIGHT, pad);
    buttons->Add(cancel);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(10));

    dialog.SetSizer(outer);
    sizeDialog(&dialog, config.width, config.height);

    if (dialog.ShowModal() == wxID_APPLY && onApply) {
        onApply(static_cast<int>(junk->getValue() + 0.5),
                static_cast<int>(junkPerLevel->getValue() + 0.5),
                static_cast<int>(level->getValue() + 0.5));
    }
}

// ============================================================================
// Volume control
// ============================================================================

void createVolumeControlDialog(wxWindow* parent,
                               const VolumeControlConfig& config,
                               std::function<void(int)> onSfxVolumeChanged,
                               std::function<void(int)> onMusicVolumeChanged) {
    wxDialog dialog(parent, wxID_ANY, U(config.title));
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    int pad = dialog.FromDIP(5);
    bool retro = config.isRetroMode;

    if (retro) {
        wxStaticText* header = new wxStaticText(&dialog, wxID_ANY, wxEmptyString);
        header->SetLabelMarkup(U("<span size='x-large' weight='bold'>★ РЕГУЛИРОВКА ГРОМКОСТИ ★</span>"));
        vbox->Add(header, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, pad);
        vbox->Add(new wxStaticText(&dialog, wxID_ANY, U("ПРОТОКОЛ ЗВУКОВОГО КОНТРОЛЯ № 1984/ZB-3")),
                  0, wxALIGN_CENTER_HORIZONTAL | wxALL, pad);
    }

    vbox->Add(new wxStaticText(&dialog, wxID_ANY,
                               U(retro ? "ГРОМКОСТЬ ЭФФЕКТОВ ГОСУДАРСТВЕННОЙ ВАЖНОСТИ:" : "Sound Effects Volume:")),
              0, wxTOP, pad);
    ValueSlider* sfx = new ValueSlider(&dialog, 0, 100, 1, config.sfxVolume, 0);
    sfx->onChange([onSfxVolumeChanged](double v) {
        if (onSfxVolumeChanged) onSfxVolumeChanged(static_cast<int>(v + 0.5));
    });
    vbox->Add(sfx, 0, wxEXPAND);
    vbox->Add(rangeCaptions(&dialog, retro ? "ОТКЛЮЧЕНО" : "Mute",
                            retro ? "МАКСИМАЛЬНАЯ ГРОМКОСТЬ" : "Max"), 0, wxEXPAND);

    vbox->Add(new wxStaticLine(&dialog), 0, wxEXPAND | wxTOP | wxBOTTOM, pad * 2);

    vbox->Add(new wxStaticText(&dialog, wxID_ANY,
                               U(retro ? "ГРОМКОСТЬ ПАТРИОТИЧЕСКОЙ МУЗЫКИ:" : "Music Volume:")),
              0, 0);
    ValueSlider* music = new ValueSlider(&dialog, 0, 100, 1, config.musicVolume, 0);
    music->onChange([onMusicVolumeChanged](double v) {
        if (onMusicVolumeChanged) onMusicVolumeChanged(static_cast<int>(v + 0.5));
    });
    vbox->Add(music, 0, wxEXPAND);
    vbox->Add(rangeCaptions(&dialog, retro ? "ТИШИНА" : "Mute",
                            retro ? "СЛАВА РОДИНЕ!" : "Max"), 0, wxEXPAND);

    outer->Add(vbox, 1, wxEXPAND | wxALL, dialog.FromDIP(15));
    wxButton* ok = new wxButton(&dialog, wxID_OK, U(convertMnemonic(config.okButtonLabel)));
    ok->SetDefault();
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    buttons->Add(ok);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(15));

    dialog.SetSizer(outer);
    dialog.SetEscapeId(wxID_OK);
    sizeDialog(&dialog, config.width, config.height);
    dialog.ShowModal();
}

// ============================================================================
// Generic slider settings dialog (block size, game size, block trails)
// ============================================================================

bool createSliderSettingsDialog(wxWindow* parent,
                                const std::string& title,
                                const std::string& intro,
                                std::vector<SliderSpec>& sliders,
                                const std::string& note,
                                int width,
                                int height) {
    wxDialog dialog(parent, wxID_ANY, U(title));
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    int pad = dialog.FromDIP(5);

    if (!intro.empty()) {
        vbox->Add(new wxStaticText(&dialog, wxID_ANY, U(intro)), 0, wxBOTTOM, pad);
    }

    std::vector<ValueSlider*> controls;
    for (const SliderSpec& spec : sliders) {
        wxWindow* owner = &dialog;
        wxSizer* target = vbox;
        wxStaticBoxSizer* box = nullptr;
        if (!spec.frameTitle.empty()) {
            box = new wxStaticBoxSizer(wxVERTICAL, &dialog, U(spec.frameTitle));
            owner = box->GetStaticBox();
            target = box;
        }
        int digits = spec.step < 1.0 ? 2 : 0;
        ValueSlider* slider = new ValueSlider(owner, spec.minValue, spec.maxValue, spec.step,
                                              spec.value, digits, spec.valueFormat);
        target->Add(slider, 0, wxEXPAND | wxALL, pad);
        if (!spec.leftCaption.empty() || !spec.rightCaption.empty()) {
            target->Add(rangeCaptions(owner, spec.leftCaption, spec.rightCaption),
                        0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, pad);
        }
        if (box) vbox->Add(box, 0, wxEXPAND | wxBOTTOM, pad * 2);
        controls.push_back(slider);
    }

    if (!note.empty()) {
        vbox->Add(new wxStaticText(&dialog, wxID_ANY, U(note)), 0, wxTOP, pad * 2);
    }

    outer->Add(vbox, 1, wxEXPAND | wxALL, dialog.FromDIP(10));

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* apply = new wxButton(&dialog, wxID_APPLY, "&Apply");
    wxButton* cancel = new wxButton(&dialog, wxID_CANCEL, "&Cancel");
    apply->SetDefault();
    apply->Bind(wxEVT_BUTTON, [&dialog](wxCommandEvent&) { dialog.EndModal(wxID_APPLY); });
    buttons->AddStretchSpacer();
    buttons->Add(apply, 0, wxRIGHT, pad);
    buttons->Add(cancel);
    outer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dialog.FromDIP(10));

    dialog.SetSizer(outer);
    sizeDialog(&dialog, width, height);

    if (dialog.ShowModal() != wxID_APPLY) return false;
    for (size_t i = 0; i < sliders.size(); ++i) sliders[i].value = controls[i]->getValue();
    return true;
}

}  // namespace WXHelpers
