#pragma once
#include "DFVComponent.h"

class ComplexPlaneEditor final
  :public DFVComponent
  ,private juce::ValueTree::Listener
{
  /** TODO(ry):
   * better root creation interface (ability to create poles, increase/decrease root order)
   * split roots
   * visually indicate snaping roots to real axis
   * visually indicate when a root is being hovered over (highlight, show tooltip)
   * restore a root to its previous position when undoing its deletion
   * improve axis label positioning relative to grid lines
   */

public:
  ComplexPlaneEditor(AudioPluginAudioProcessor *p);
  ~ComplexPlaneEditor();

  class RootPoint final
    : public juce::Component,
      private juce::ValueTree::Listener,
      private juce::Timer
  {
  public:
    RootPoint(bool c, FilterRoot::Ptr r);
    ~RootPoint();

    /**
     * move point to a new value determined by a screen-space position relative
     * to this component.
     */
    void moveToLocalSpace(juce::Point<float> localPositionScreen);
    /**
     * move point to a new value determined by a screen-space position relative
     * to the editor component.
     */
    void moveToEditorSpace(juce::Point<double> editorPositionScreen);
    /**
     * move point to new value, preserving filter stability and handling
     * snapping to axis, and set the target point for merging.
     */
    void moveToWorldSpace(c128 newRootValue);

    c128 moveRoot(c128 newRootValue) { return(editor->processor->filterState->moveRoot(root, newRootValue)); }
    c128 moveRoot(r64 newRootValueRe, r64 newRootValueIm) { return(moveRoot(c128(newRootValueRe, newRootValueIm))); }

    void mouseEnter(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;

    void paint(juce::Graphics &g) override;

    void updateBounds(c128 value);

    bool isConjugate;
    FilterRoot::Ptr root;
    RootPoint *conjugate;


  private:
    void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override;
    void timerCallback(void) override;

    static s32 constexpr timerFreq = 50;

    friend ComplexPlaneEditor;
    static ComplexPlaneEditor *editor;
    c128 valueAtDragStart;

    // TODO(ry): smells generatable...
    enum InteractionFlags : u32 {
      InteractionFlag_isDragging            = 1 << 0,
      InteractionFlag_wasRightButtonDown    = 1 << 1,
    };

    b32 isDragging(void) { return flags & InteractionFlag_isDragging; }
    b32 wasRightButtonDown(void) { return flags & InteractionFlag_wasRightButtonDown; }

    void isDragging(b32 set)
    { set ? flags |= InteractionFlag_isDragging : flags &= ~InteractionFlag_isDragging; }
    void wasRightButtonDown(b32 set)
    { set ? flags |= InteractionFlag_wasRightButtonDown : flags &= ~InteractionFlag_wasRightButtonDown; }

    u32 flags;
  };

  class RootTooltip final : public juce::Component, private juce::Timer
  {
  public:
    explicit RootTooltip(ComplexPlaneEditor *e);

    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    void resized(void) override;

    void paint(juce::Graphics &g) override;

    inline void setText(juce::String const &text)
    {
      orderLabel.setText(text, juce::dontSendNotification);
    }

    inline void setPos(juce::Point<int> point)
    {
      setTopRightPosition(point.getX() + (this->getWidth()  + 10),
			  point.getY() - (this->getHeight() - 10));
    }

    inline void setRoot(FilterRoot::Ptr r)
    {
      root = r;
      if(auto *rootPtr = root.get())
      {
	setText(juce::String(rootPtr->order));
      }
    }

    inline void show()
    {
      keepaliveCounter += 1;
      setVisible(true);
    }

    inline void setRootAndShow(FilterRoot::Ptr r)
    {
      setRoot(r);
      show();
    }

    inline void setPointAndShow(RootPoint *point)
    {
      setPos(juce::Point<int>(point->getX(), point->getY()));
      setRootAndShow(point->root);
    }

    inline void hide(void)
    {
      if(keepaliveCounter)
      {
	if(!isTimerRunning())
	{
	  startTimer(keepaliveDelayMs);
	}
	++pendingEventCount;
      }
    }

  private:
    void timerCallback(void) override
    {
      DBG("tooltip timer callback: keepalive = " << keepaliveCounter);
      keepaliveCounter -= 1;
      if(!(--pendingEventCount))
      {
	stopTimer();
      }
      if(keepaliveCounter <= 0)
      {
	setVisible(false);
      }
    }

    FilterRoot::Ptr root;

    juce::TextButton destroy;
    juce::TextButton orderInc;
    juce::TextButton orderDec;
    juce::Label orderLabel;

    s32 keepaliveCounter = 0;
    s32 pendingEventCount = 0;
    static s32 const keepaliveDelayMs = 60;

    friend ComplexPlaneEditor;
    ComplexPlaneEditor *editor;
  };

  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  void resized(void) override;

  void paint(juce::Graphics &g) override;

  void setPointHoveringOverAxis(bool hovering);
  bool isPointHoveringOverAxis(void);

  juce::AffineTransform pixelsFromWorldUnits;
  juce::AffineTransform worldUnitsFromPixels;

  // TODO(ry): tune
  static constexpr r64 snapThresholdPixels  = 18.0;
  static constexpr r64 stickThresholdPixels = snapThresholdPixels / 2.0;

  enum InteractionFlags : u32 {
    InteractionFlag_constantMagnitude = 1 << 0,
    InteractionFlag_constantAngle     = 1 << 1,
  };

  b32 constantMagnitudeInteraction(void)
  { return transientInteractionFlags & InteractionFlag_constantMagnitude; }
  b32 constantAngleInteraction(void)
  { return transientInteractionFlags & InteractionFlag_constantAngle; }

  void setInteractionMode(u32 iflags, bool authoritative = false)
  {
    if(authoritative)
    {
      authoritativeInteractionFlags = iflags;
      transientInteractionFlags = authoritativeInteractionFlags;
    }
    else
    {
      transientInteractionFlags = iflags|authoritativeInteractionFlags;

      constantMagnitudeInteractionButton
	.setToggleState(constantMagnitudeInteraction(), juce::dontSendNotification);
      constantAngleInteractionButton
	.setToggleState(constantAngleInteraction(), juce::dontSendNotification);
      defaultInteractionButton
	.setToggleState(!constantMagnitudeInteraction() && !constantAngleInteraction(),
			juce::dontSendNotification);
    }

    DBG("setter:");
    DBG("constantMagnitude: " << (constantMagnitudeInteraction() ? "set" : "unset"));
    DBG("constantAngle: " << (constantAngleInteraction() ? "set" : "unset"));
  }

private:
  void updateTransforms(void);
  void updateTransformsAndChildBounds(void);

  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override;

  double pixelsPerUnit;
  double unitsPerPixel;
  double unitsPerLine;

  AudioPluginAudioProcessor *processor;

  juce::Point<double> worldCenter; // NOTE(ry): the center of the drawable region in world coordinates
  juce::Point<double> worldCenterAtDragStart;
  juce::Rectangle<int> plotRect;

  juce::OwnedArray<RootPoint> points;

  RootTooltip tooltip;

  // interaction mode toggles
  juce::TextButton defaultInteractionButton;
  juce::TextButton constantMagnitudeInteractionButton;
  juce::TextButton constantAngleInteractionButton;

  u32 authoritativeInteractionFlags;
  u32 transientInteractionFlags;

  static constexpr int InteractionToggleGroupID = 0x00867068;

  bool pointHoveringOverAxis;
};
