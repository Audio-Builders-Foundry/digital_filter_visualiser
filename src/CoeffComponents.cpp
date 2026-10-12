#include "CoeffComponents.h"
#include "RootsToCoefficients.h"
#include "CoefficientsToRoots.cpp"

#include <iostream>
#define COL_WIDTH 100 // TODO make this value relate to the global UI.

CoefficientsComponent::CoefficientsComponent(AudioPluginAudioProcessor* p)
    : processor(p)
    , recalculateButton("RECALCULATE")
{
    //setupCoefficient's table
    setName("FILTER COEFFICIENTS");
    coeffTable.setModel(this); // pass ownership to the class.
    addAndMakeVisible(coeffTable);
    coeffTable.setHeader(std::make_unique<CoeffTableHeader>());
    juce::TableHeaderComponent& tbComp = coeffTable.getHeader();
    tbComp.addColumn("Delay", 1, int(COL_WIDTH/2.5));
    tbComp.addColumn("FeedForward", 2, int(COL_WIDTH));
    tbComp.addColumn("FeedBack", 3, int(COL_WIDTH));
    tbComp.setStretchToFitActive(true); // expand columns to fill the entire width of the component

    recalculateButton.setEnabled(false);
    recalculateButton.onClick = [this] { this->updateFilterState(); };
    addAndMakeVisible(recalculateButton);

    processor->filterState->addListener(this);
    processor->filterState->syncListener(this);
}

CoefficientsComponent::~CoefficientsComponent()
{
    coeffTable.setModel(nullptr);
}

// Note:    Apart from any other reasons that may occur throughout future dev updates,..
//          ..in case that the labels created at refreshComponentForCell have to be properly deallocated,..
//          ..(See JUCE doc: https://docs.juce.com/master/classjuce_1_1TableListBoxModel.html#a873b2b84429f026ea046528af1f4fe81)..
//          ..the destructor should be defined, and consequently align explicitly to the rule of 5.
// CoefficientsComponent::~CoefficientsComponent() {}

void CoefficientsComponent::resized()
{
    PROFILE_FUNCTION();

    DFVComponent::resized();
    // TODO adjust dims and position based on area or neighbor components --> should call something like getChildren() + getName() but seems fragile quering approach
    constexpr int recalculateButtonHeight = 25;
    //headerRect.setBounds(area.getX(), area.getY(), static_cast<float>(area.getWidth()), headerHeight);
    coeffTable.setBounds(reducedBounds.getX(), reducedBounds.getY(), reducedBounds.getWidth(), reducedBounds.getHeight() - recalculateButtonHeight - 3 * padding);
    recalculateButton.setBounds(reducedBounds.getX() + 4 * padding, coeffTable.getBottom() + 2 * padding, reducedBounds.getWidth() - 8 * padding, recalculateButtonHeight);
}

int CoefficientsComponent::getNumRows()
{
    return static_cast<int>(fbcoeffs.size());
}

void CoefficientsComponent::paintRowBackground(juce::Graphics& g, int row, int w, int h, bool rowIsSelected)
{
    PROFILE_FUNCTION();

    juce::ignoreUnused(row);
    juce::ignoreUnused(w);
    juce::ignoreUnused(h);

    // TODO fix line can t become unselected, but only when other line is selected
    if (rowIsSelected)
    {
        g.fillAll(findColour(DFVLookAndFeel::tableSelectedRowBackgroundId));
    }
}

void CoefficientsComponent::paintCell(juce::Graphics& g, int row, int col, int w, int h, bool)
{
    PROFILE_FUNCTION();

    if (row >= getNumRows()) return;

    g.setColour(findColour(juce::TableListBox::ColourIds::outlineColourId));
    g.drawRect(0, 0, w, h);

    juce::String text;
    if (col == 1) text = juce::String(row);
    if (col == 2) text = juce::String(ffcoeffs[static_cast<size_t>(row)]);
    if (col == 3) text = juce::String(fbcoeffs[static_cast<size_t>(row)]);

    g.drawText(text, 0, 0, w, h, juce::Justification::centredLeft);
}

juce::Component* CoefficientsComponent::refreshComponentForCell(int row, int col, bool, juce::Component* existing)
{

    if (col !=2 && col !=3 )
        return nullptr;

    // creating new cells
    auto* label = static_cast<juce::Label*>(existing);
    if (!label)
    {
        // Create a new editable Label restricting user from invalid input

        label = new juce::Label();

        label->onEditorShow = [label] {
            // restrict input when editor apears
            if (auto* editor = label->getCurrentTextEditor())
                editor->setInputRestrictions(0, "0123456789.-");
            };

        // update data when editing finishes
        label->onTextChange = [this, row, col, label]{
            double value = label->getText().getDoubleValue();
            onCellChanged(row, col, value);
            };
    }

    label->setEditable(true, true, false);
    label->setColour(juce::Label::outlineColourId, findColour(juce::TableListBox::ColourIds::outlineColourId));
    label->setColour(juce::Label::textColourId, findColour(juce::TableListBox::ColourIds::textColourId));

    // update value prevernting dump data from appearing in case of a vector size mismatch
    double value;
    if (col == 2)
    {
        value = ffcoeffs[static_cast<size_t>(row)];
    }
    else // col == 3
    {
        jassert(col == 3);
        value = fbcoeffs[static_cast<size_t>(row)];
    }
    label->setText(juce::String(value), juce::dontSendNotification );
    return label;
}

void CoefficientsComponent::onCellChanged(int row, int col, double value)
{
    // calculate roots through the coefficient2roots function && notify other listeners about this change
    if (col == 2)
    {
        ffcoeffs[static_cast<size_t>(row)] = value;
        isFeedForwardSynchronized = false;
        recalculateButton.setEnabled(true);
    }
    else if (col == 3)
    {
        fbcoeffs[static_cast<size_t>(row)] = value;
        isFeedBackSynchronized = false;
        recalculateButton.setEnabled(true);
    }
}

void CoefficientsComponent::updateFilterState()
{
    if (!isFeedForwardSynchronized)
    {
        auto zeros = Solve(this->ffcoeffs);

        while (!processor->filterState->zeros.isEmpty())
        {
            auto *r = processor->filterState->zeros.getFirst();
            processor->filterState->remove(r);
        }

        for (size_t i=0 ; i< zeros.size(); i++)
        {
            auto &r = zeros[i].value;
            auto &order = zeros[i].order;
            processor->filterState->add(order, r);
        }
    }
    if (!isFeedBackSynchronized)
    {
        auto poles = Solve(this->fbcoeffs);

        // check stability
        auto filterStable = [&]() -> bool {
            for (auto& [r, order] : poles)
            {
                const double re = r.real();
                const double im = r.imag();
                const double magnitude { std::sqrt ( re * re + im * im) };
                if ( magnitude >= processor->filterState->maxPoleMagnitude)
                    return false;
            }
            return true;
            };

        if (!filterStable())
        {
            // @TODO handle that case, reporting back to the user a message about the causality violation
            // unstable filter, do nothing & return
            return;
        }


        size_t prev_sz = static_cast<size_t>(processor->filterState->poles.size());

        for (size_t i=0 ; i< poles.size(); i++)
        {

            auto &r = poles[i].value;
            auto &order = poles[i].order;
            processor->filterState->add(-order, r);
        }

        auto isNewPole = [&](c128 val) -> std::pair<bool, int> {
            for (auto& [r, order] : poles)
            {
                if (val == r)
                    return {true, order};
            }
            return {false, 0};
        };

        for (size_t i=0 ; i< prev_sz; i++)
        {
            auto *r = processor->filterState->poles[static_cast<int>(i)];
            c128 key(c128(r->value.re.get(),r->value.im.get()));
            auto [found, order] = isNewPole(key);

            if ( found )
            {
                r->order= -order;
            }
            else
            {
                processor->filterState->remove(r);
                i--;
                prev_sz--;
            }
        }
    }
    isFeedBackSynchronized = isFeedForwardSynchronized = true;
    recalculateButton.setEnabled(false);
}

void CoefficientsComponent::valueTreePropertyChanged (juce::ValueTree& node, const juce::Identifier& property)
{
    if(property == IDs::ValueRe || property == IDs::ValueIm)    // when dragging roots
    {
        updateCoeffTable();
    }
    else if (property == IDs::Order)    // when changing order of a root
    {
        int order = node.getProperty(IDs::Order);
        if(order != 0)
        {
            updateCoeffTable();
        }
    }
}

void CoefficientsComponent::valueTreeChildAdded (juce::ValueTree& node, juce::ValueTree& child)
{
    juce::ignoreUnused(node);
    juce::ignoreUnused(child);

    updateCoeffTable();
}

void CoefficientsComponent::valueTreeChildRemoved (juce::ValueTree& node, juce::ValueTree& child, int idx)
{
    juce::ignoreUnused(node);
    juce::ignoreUnused(child);
    juce::ignoreUnused(idx);

    if (processor->filterState->totalOrder==0) // if filter is cleaned out by erasing the last root
    {
        ffcoeffs.clear();
        fbcoeffs.clear();
        coeffTable.updateContent();
        return;
    }
    updateCoeffTable();
}

void CoefficientsComponent::updateCoeffTable()
{
    fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
        processor->filterState->poles);
    ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
        processor->filterState->zeros, static_cast<int>(fbcoeffs.size()));
    coeffTable.updateContent();
}

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
