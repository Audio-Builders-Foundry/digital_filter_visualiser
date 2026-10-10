#pragma once

#include "FilterState.h"
#include "CoefficientsToRoots.h"

#include <vector>
#include <string>

#define TABLE_TITLE "Coefficients"

class CoefficientsComponent final
    : public juce::Component
    , private juce::TableListBoxModel
    , private juce::ValueTree::Listener
{
    public:
        CoefficientsComponent(AudioPluginAudioProcessor*);
        ~CoefficientsComponent();
        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        static CoefficientsToRoots::SolverFn constexpr Solve = QRSolve;

        std::vector<double> ffcoeffs;
        std::vector<double> fbcoeffs;
        juce::Label titleLabel;
        juce::TableListBox coeffTable;
        AudioPluginAudioProcessor *processor;

        void toggleCollapseExpand();
        void updateCoeffTable();
        void updateFilterStateOnCoefEdit(int row, int col, double value);

        // override juce::TableListBox
        int getNumRows() override;
        void paintRowBackground(juce::Graphics&, int , int, int, bool) override;
        void paintCell(juce::Graphics&, int , int, int, int, bool) override; // method for updating columns
        juce::Component* refreshComponentForCell(int, int, bool, juce::Component*) override; // method for creating/updating single cells

        // override juce::ValueTree::Listener
        void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
        void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
        void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
        // will be used for single cell edits
        // void sendPropertyChangeMessage (const Identifier& property);

        class CoeffTableHeader
            : public juce::TableHeaderComponent
        {
            public:
	        // NOTE(ry): columns are not clickable
	        // TODO(ry): make columns not highlight on mouse hover, since they are not clickable
	        void columnClicked(int, juce::ModifierKeys const &) override {}
        };
};
