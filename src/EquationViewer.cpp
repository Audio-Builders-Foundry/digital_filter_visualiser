EquationViewer::
EquationViewer(AudioPluginAudioProcessor *p)
  :processor(p)
{
  addAndMakeVisible(numeratorText);
  addAndMakeVisible(denominatorText);

  numeratorText.setViewport(&numeratorView);
  denominatorText.setViewport(&denominatorView);

  // numeratorView.setViewedComponent(&numeratorText, false);
  // denominatorView.setViewedComponent(&denominatorText, false);
  numeratorView.setScrollBarThickness(scrollBarHeightPixels);
  denominatorView.setScrollBarThickness(scrollBarHeightPixels);

  addAndMakeVisible(numeratorView);
  addAndMakeVisible(denominatorView);

  processor->filterState->addListener(this);
  processor->filterState->syncListener(this);
}

EquationViewer::
~EquationViewer()
{
  processor->filterState->removeListener(this);
}

void EquationViewer::
resized(void)
{
  auto area = getLocalBounds();
  auto constexpr margin = 5;
  auto numeratorBounds = area.removeFromTop(int(0.5*getHeight())).reduced(margin);
  auto denominatorBounds = area.removeFromTop(int(0.5*getHeight())).reduced(margin);
  numeratorView.setBounds(numeratorBounds);
  denominatorView.setBounds(denominatorBounds);

  auto constexpr maxTextWidth = 12000;
  numeratorBounds.removeFromBottom(scrollBarHeightPixels);
  denominatorBounds.removeFromBottom(scrollBarHeightPixels);
  numeratorText.setBounds(numeratorBounds.reduced(margin).withWidth(maxTextWidth));
  denominatorText.setBounds(denominatorBounds.reduced(margin).withWidth(maxTextWidth));
}

void EquationViewer::
paint(juce::Graphics &g)
{
  auto constexpr margin = 5;
  auto constexpr barThicknessPixels = 2;
  g.setColour(findColour(DFVLookAndFeel::equationTextColourId));
  g.fillRect(margin, int(0.5*getHeight()), getWidth() - margin, barThicknessPixels);
}

void EquationViewer::
valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child)
{
  juce::ignoreUnused(parent);
  juce::ignoreUnused(child);

  updateCoeffs();
}

void EquationViewer::
valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index)
{
  juce::ignoreUnused(parent);
  juce::ignoreUnused(child);
  juce::ignoreUnused(index);

  updateCoeffs();
}

void EquationViewer::
valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property)
{
  if(property == IDs::ValueRe || property == IDs::ValueIm)
  {
    updateCoeffs();
  }
  else if(property == IDs::Order)
  {
    int order = node.getProperty(IDs::Order);
    if(order != 0)
    {
      // NOTE(ry): if order is being set to zero, the node will get removed and
      // update will be handled in the removed callback
      updateCoeffs();
    }
  }
}

void EquationViewer::
updateCoeffs(void)
{
  auto fbCoeffs =
    RootsToCoefficients::CalculatePolynomialCoefficientsFrom(processor->filterState->poles);
  auto ffCoeffs =
    RootsToCoefficients::CalculatePolynomialCoefficientsFrom(processor->filterState->zeros,
                                                             int(fbCoeffs.size()));

  denominatorText.setTextFromCoeffs(fbCoeffs);
  numeratorText.setTextFromCoeffs(ffCoeffs,
				  fbCoeffs.size() - ffCoeffs.size());

  repaint();
}

void EquationViewer::EquationText::
mouseWheelMove(juce::MouseEvent const &e, juce::MouseWheelDetails const &w)
{
  juce::ignoreUnused(e);

  auto constexpr scrollAmp = 50;
  auto const viewPosition = viewport->getViewPosition();
  auto const newX = viewPosition.x + scrollAmp*w.deltaY;
  auto const newY = viewPosition.y;
  viewport->setViewPosition(int(newX), int(newY));
}

void EquationViewer::EquationText::
paint(juce::Graphics &g)
{
  auto const fontHeightPixels = std::clamp(getHeight() - 2.0, 2.0, 24.0);
  g.setColour(findColour(DFVLookAndFeel::equationTextColourId));
  g.setFont(fontHeightPixels);
  auto font = g.getCurrentFont();

  // NOTE(ry): create glyph arrangement and add non-escaped text to it
  juce::GlyphArrangement arr{};
  auto const bottomLeft = getLocalBounds().getBottomLeft();
  arr.addLineOfText(font, text.removeCharacters("^v"), bottomLeft.x, bottomLeft.y);

  // NOTE(ry): find exponent escape characters to determine which glyphs to shift
  struct ExpRun {
    int start, count;
  };
  std::vector<ExpRun> expRuns;
  {
    auto parseText = text;
    int parsed = 0;
    while(parseText.length())
    {
      auto expStart = parseText.indexOfChar('^');
      auto expEnd = parseText.indexOfChar('v');
      if(expStart == -1) break;
      if(expEnd == -1) expEnd = text.length();
      auto expCount = expEnd - expStart;

      auto glyphStartIdx = parsed + expStart;
      auto glyphCount = expCount - 1;
      expRuns.push_back({glyphStartIdx, glyphCount});

      parseText = parseText.substring(expEnd+1);
      parsed += expEnd - 1;
    }
  }

  // NOTE(ry): shift exponent glyphs
  auto const deltaY = -fontHeightPixels/3.0;
  for(auto run : expRuns)
  {
    arr.moveRangeOfGlyphs(run.start, run.count, 0, deltaY);
  }

  // NOTE(ry): resize component according to new text size
  auto const bb = arr.getBoundingBox(0, arr.getNumGlyphs(), true);
  setBounds(getBounds().withWidth(std::max(getParentWidth(),
                                  bb.getSmallestIntegerContainer().getWidth())));

  // NOTE(ry): center and draw text
  arr.justifyGlyphs(0, arr.getNumGlyphs(),
                    bb.getX(), bb.getY(), getBounds().getWidth(), bb.getHeight(),
                    juce::Justification(juce::Justification::horizontallyCentred));
  arr.draw(g);
}

void EquationViewer::EquationText::
setViewport(juce::Viewport *v)
{
  viewport = v;
  viewport->setViewedComponent(this, false);
}

void EquationViewer::EquationText::
setTextFromCoeffs(std::vector<double> const &coeffs, size_t firstNonzeroIndex)
{
  auto constexpr numDecimalPlaces = 2;

  text.clear();
  for(size_t idx = firstNonzeroIndex; idx < coeffs.size(); ++idx)
  {
    auto const coeff = coeffs[idx];
    auto const order = -int(idx);
    if(!juce::approximatelyEqual(coeff, 0.0))
    {
      // TODO(ry): add whitespace, truncate nearly-integer coeffs, omit if coeff ~= 1
      if(coeff > 0.0 && idx > firstNonzeroIndex)
      {
        text << "+";
      }
      text << juce::String(coeff, numDecimalPlaces);
      if(order != 0)
      {
        // NOTE(ry): the exponent is between the '^' and 'v' characters. the
        // escape characters are only used to find the range of exponent
        // glyphs. they are removed when drawing the text
        text << "z^" << juce::String(order) << "v";
      }
    }
  }
}
