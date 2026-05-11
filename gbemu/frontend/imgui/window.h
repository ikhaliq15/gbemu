#ifndef GBEMU_FRONTEND_IMGUI_WINDOW_H
#define GBEMU_FRONTEND_IMGUI_WINDOW_H

namespace gbemu::frontend
{

class ImguiWindow
{
  public:
    ImguiWindow() = default;
    ~ImguiWindow() = default;

    void setVisible(bool visible) { visible_ = visible; }
    void toggleVisibility() { visible_ = !visible_; }
    bool isVisible() const { return visible_; }

  protected:
    bool visible_ = false;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_WINDOW_H
