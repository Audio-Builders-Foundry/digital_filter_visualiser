#include "DFVComponent.h"

class EquationViewer
  :public DFVComponent
  ,private juce::ValueTree::Listener
{
public:
  explicit EquationViewer(AudioPluginAudioProcessor *p);
  ~EquationViewer();

  void resized(void) override;
  void paint(juce::Graphics &g) override;

private:
  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override;
  void valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property) override;
  void updateCoeffs(void);

  class EquationText : public juce::Component
  {
  public:
    void mouseWheelMove(juce::MouseEvent const &e, juce::MouseWheelDetails const &w) override;
    void paint(juce::Graphics &g) override;
    void setViewport(juce::Viewport *v);

  private:
    void setTextFromCoeffs(std::vector<double> const &coeffs, size_t firstNonzeroIndex = 0);

    friend EquationViewer;
    juce::Viewport *viewport;
    juce::String text;
  };

  AudioPluginAudioProcessor *processor;

  EquationText numeratorText, denominatorText;
  juce::Viewport numeratorView, denominatorView;

  static auto constexpr margin = 2;
  static auto constexpr labelWidth = 100;
  static auto constexpr scrollBarHeightPixels = 5;
};
