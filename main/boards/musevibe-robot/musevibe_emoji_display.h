#pragma once

#include "display/lcd_display.h"

/**
 * @brief MuseVibe机器人GIF表情显示类
 * 继承SpiLcdDisplay，通过EmojiCollection添加GIF表情支持
 */
class MuseVibeEmojiDisplay : public SpiLcdDisplay {
   public:
    /**
     * @brief 构造函数，参数与SpiLcdDisplay相同
     */
    MuseVibeEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy);

    virtual ~MuseVibeEmojiDisplay() = default;
    virtual void SetStatus(const char* status) override;
    virtual void SetPreviewImage(std::unique_ptr<LvglImage> image) override;
    virtual void SetupUI() override;
    virtual void UpdateStatusBar(bool update_all = false) override;

   private:
    void InitializeMuseVibeEmojis();
    void SetupPreviewImage();
    bool battery_label_was_red_ = false;
};