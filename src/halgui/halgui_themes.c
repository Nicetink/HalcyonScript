/*
 * HalGUI - Modern Theme System
 * 
 * Material Design 3 / JetBrains inspired themes
 * Clean, minimal, professional enterprise-grade design
 * 
 * Design principles:
 * - 8px grid system
 * - Soft shadows and elevation
 * - Rounded corners (8-12px)
 * - Subtle color palette
 * - Clear visual hierarchy
 */

#include "halgui.h"

/* ============================================
   HAL_THEME_LIGHT - Modern Light Theme
   
   Inspired by Material Design 3 Light
   Clean, professional, enterprise-grade
   ============================================ */

HalTheme HAL_THEME_LIGHT = {
    // Primary - Soft blue accent
    .primary         = HAL_RGB(66, 133, 244),    // Google Blue
    .primaryHover    = HAL_RGB(53, 122, 232),    // Slightly darker
    .primaryActive   = HAL_RGB(41, 110, 220),    // Pressed state
    
    // Background - Warm neutral
    .background      = HAL_RGB(250, 250, 252),   // Off-white, not pure white
    .surface         = HAL_RGB(255, 255, 255),   // Cards/panels
    .surfaceHover    = HAL_RGB(245, 247, 250),   // Hover state
    .elevated        = HAL_RGB(255, 255, 255),   // Elevated surfaces
    
    // Text - High contrast but not harsh
    .textPrimary     = HAL_RGB(32, 33, 36),      // Near black
    .textSecondary   = HAL_RGB(95, 99, 104),     // Muted gray
    .textDisabled    = HAL_RGB(175, 180, 185),   // Disabled
    .textOnPrimary   = HAL_RGB(255, 255, 255),   // White on primary
    
    // Borders - Subtle
    .border          = HAL_RGB(218, 220, 224),   // Light gray
    .borderFocus     = HAL_RGB(66, 133, 244),    // Primary on focus
    .borderError     = HAL_RGB(234, 67, 53),     // Error red
    
    // Status colors - Material palette
    .success         = HAL_RGB(52, 168, 83),     // Green
    .warning         = HAL_RGB(251, 188, 4),     // Amber
    .error           = HAL_RGB(234, 67, 53),     // Red
    .info            = HAL_RGB(66, 133, 244),    // Blue
    
    // Shadows - Soft and subtle
    .shadowAlpha1    = 8,
    .shadowAlpha2    = 16,
    .shadowAlpha3    = 24,
    
    // Typography
    .fontFamily      = "Segoe UI",
    .fontSizeSmall   = 12,
    .fontSizeNormal  = 14,
    .fontSizeLarge   = 16,
    .fontSizeTitle   = 20,
    
    // Spacing - 8px grid
    .spacingXS       = 4,
    .spacingS        = 8,
    .spacingM        = 16,
    .spacingL        = 24,
    .spacingXL       = 32,
    
    // Border radius - Modern rounded
    .radiusSmall     = 6,
    .radiusNormal    = 10,
    .radiusLarge     = 14,
    .radiusRound     = 9999,
    
    // Animation - Smooth micro-interactions
    .animationFast   = 100,
    .animationNormal = 180,
    .animationSlow   = 280
};

/* ============================================
   HAL_THEME_DARK - Modern Dark Theme
   
   Inspired by Material Design 3 Dark
   Professional, calm, easy on eyes
   ============================================ */

HalTheme HAL_THEME_DARK = {
    // Primary - Vibrant blue on dark
    .primary         = HAL_RGB(138, 180, 248),   // Light blue
    .primaryHover    = HAL_RGB(168, 199, 250),   // Lighter on hover
    .primaryActive   = HAL_RGB(108, 160, 240),   // Darker on press
    
    // Background - Deep neutral
    .background      = HAL_RGB(25, 28, 32),      // Deep charcoal
    .surface         = HAL_RGB(35, 39, 46),      // Elevated surface
    .surfaceHover    = HAL_RGB(45, 50, 58),      // Hover state
    .elevated        = HAL_RGB(48, 53, 62),      // Dropdowns, tooltips
    
    // Text - Soft white
    .textPrimary     = HAL_RGB(232, 234, 237),   // Off-white
    .textSecondary   = HAL_RGB(154, 160, 166),   // Muted
    .textDisabled    = HAL_RGB(95, 99, 104),     // Disabled
    .textOnPrimary   = HAL_RGB(25, 28, 32),      // Dark on primary
    
    // Borders - Subtle
    .border          = HAL_RGB(60, 65, 73),      // Soft border
    .borderFocus     = HAL_RGB(138, 180, 248),   // Primary on focus
    .borderError     = HAL_RGB(242, 139, 130),   // Soft red
    
    // Status colors - Softer for dark mode
    .success         = HAL_RGB(129, 201, 149),   // Soft green
    .warning         = HAL_RGB(253, 214, 99),    // Soft amber
    .error           = HAL_RGB(242, 139, 130),   // Soft red
    .info            = HAL_RGB(138, 180, 248),   // Soft blue
    
    // Shadows - More pronounced on dark
    .shadowAlpha1    = 20,
    .shadowAlpha2    = 35,
    .shadowAlpha3    = 50,
    
    // Typography
    .fontFamily      = "Segoe UI",
    .fontSizeSmall   = 12,
    .fontSizeNormal  = 14,
    .fontSizeLarge   = 16,
    .fontSizeTitle   = 20,
    
    // Spacing - 8px grid
    .spacingXS       = 4,
    .spacingS        = 8,
    .spacingM        = 16,
    .spacingL        = 24,
    .spacingXL       = 32,
    
    // Border radius - Modern rounded
    .radiusSmall     = 6,
    .radiusNormal    = 10,
    .radiusLarge     = 14,
    .radiusRound     = 9999,
    
    // Animation
    .animationFast   = 100,
    .animationNormal = 180,
    .animationSlow   = 280
};

/* ============================================
   HAL_THEME_MIDNIGHT - Premium Dark Theme
   
   JetBrains Darcula inspired
   Deep, rich, professional
   ============================================ */

HalTheme HAL_THEME_MIDNIGHT = {
    // Primary - Soft purple accent
    .primary         = HAL_RGB(150, 130, 220),   // Soft purple
    .primaryHover    = HAL_RGB(170, 150, 235),   // Lighter
    .primaryActive   = HAL_RGB(130, 110, 200),   // Darker
    
    // Background - True dark
    .background      = HAL_RGB(18, 18, 22),      // Near black
    .surface         = HAL_RGB(28, 28, 34),      // Cards
    .surfaceHover    = HAL_RGB(38, 38, 46),      // Hover
    .elevated        = HAL_RGB(42, 42, 52),      // Elevated
    
    // Text
    .textPrimary     = HAL_RGB(220, 220, 225),   // Soft white
    .textSecondary   = HAL_RGB(140, 140, 150),   // Muted
    .textDisabled    = HAL_RGB(80, 80, 90),      // Disabled
    .textOnPrimary   = HAL_RGB(255, 255, 255),   // White
    
    // Borders
    .border          = HAL_RGB(50, 50, 60),      // Subtle
    .borderFocus     = HAL_RGB(150, 130, 220),   // Primary
    .borderError     = HAL_RGB(255, 100, 100),   // Error
    
    // Status
    .success         = HAL_RGB(120, 200, 140),
    .warning         = HAL_RGB(240, 200, 100),
    .error           = HAL_RGB(255, 100, 100),
    .info            = HAL_RGB(100, 180, 255),
    
    // Shadows
    .shadowAlpha1    = 30,
    .shadowAlpha2    = 50,
    .shadowAlpha3    = 70,
    
    // Typography
    .fontFamily      = "Segoe UI",
    .fontSizeSmall   = 12,
    .fontSizeNormal  = 14,
    .fontSizeLarge   = 16,
    .fontSizeTitle   = 20,
    
    // Spacing
    .spacingXS       = 4,
    .spacingS        = 8,
    .spacingM        = 16,
    .spacingL        = 24,
    .spacingXL       = 32,
    
    // Border radius
    .radiusSmall     = 6,
    .radiusNormal    = 10,
    .radiusLarge     = 14,
    .radiusRound     = 9999,
    
    // Animation
    .animationFast   = 100,
    .animationNormal = 180,
    .animationSlow   = 280
};

/* ============================================
   HAL_THEME_OCEAN - Calm Blue Theme
   
   Relaxed, professional, enterprise
   ============================================ */

HalTheme HAL_THEME_OCEAN = {
    // Primary - Teal accent
    .primary         = HAL_RGB(0, 150, 136),     // Teal
    .primaryHover    = HAL_RGB(0, 170, 154),     // Lighter
    .primaryActive   = HAL_RGB(0, 130, 118),     // Darker
    
    // Background - Cool blue-gray
    .background      = HAL_RGB(236, 240, 245),   // Cool light
    .surface         = HAL_RGB(255, 255, 255),   // White cards
    .surfaceHover    = HAL_RGB(240, 244, 250),   // Hover
    .elevated        = HAL_RGB(255, 255, 255),   // Elevated
    
    // Text
    .textPrimary     = HAL_RGB(38, 50, 56),      // Blue-gray dark
    .textSecondary   = HAL_RGB(96, 108, 118),    // Muted
    .textDisabled    = HAL_RGB(158, 168, 176),   // Disabled
    .textOnPrimary   = HAL_RGB(255, 255, 255),   // White
    
    // Borders
    .border          = HAL_RGB(200, 210, 220),   // Cool gray
    .borderFocus     = HAL_RGB(0, 150, 136),     // Teal
    .borderError     = HAL_RGB(229, 57, 53),     // Red
    
    // Status
    .success         = HAL_RGB(67, 160, 71),
    .warning         = HAL_RGB(255, 167, 38),
    .error           = HAL_RGB(229, 57, 53),
    .info            = HAL_RGB(3, 169, 244),
    
    // Shadows
    .shadowAlpha1    = 10,
    .shadowAlpha2    = 18,
    .shadowAlpha3    = 26,
    
    // Typography
    .fontFamily      = "Segoe UI",
    .fontSizeSmall   = 12,
    .fontSizeNormal  = 14,
    .fontSizeLarge   = 16,
    .fontSizeTitle   = 20,
    
    // Spacing
    .spacingXS       = 4,
    .spacingS        = 8,
    .spacingM        = 16,
    .spacingL        = 24,
    .spacingXL       = 32,
    
    // Border radius
    .radiusSmall     = 6,
    .radiusNormal    = 10,
    .radiusLarge     = 14,
    .radiusRound     = 9999,
    
    // Animation
    .animationFast   = 100,
    .animationNormal = 180,
    .animationSlow   = 280
};


/* ============================================
   HAL_THEME_TEAL - Pure Teal Theme
   
   Beautiful teal/turquoise color scheme
   Modern, fresh, vibrant
   ============================================ */

HalTheme HAL_THEME_TEAL = {
    // Primary - Pure Teal
    .primary         = HAL_RGB(0, 188, 212),     // Cyan/Teal
    .primaryHover    = HAL_RGB(0, 210, 235),     // Lighter
    .primaryActive   = HAL_RGB(0, 166, 190),     // Darker
    
    // Background - Soft teal tint
    .background      = HAL_RGB(232, 245, 247),   // Very light teal
    .surface         = HAL_RGB(255, 255, 255),   // White cards
    .surfaceHover    = HAL_RGB(224, 242, 245),   // Teal tint hover
    .elevated        = HAL_RGB(255, 255, 255),   // Elevated
    
    // Text
    .textPrimary     = HAL_RGB(0, 77, 86),       // Dark teal
    .textSecondary   = HAL_RGB(69, 123, 131),    // Muted teal
    .textDisabled    = HAL_RGB(150, 180, 185),   // Disabled
    .textOnPrimary   = HAL_RGB(255, 255, 255),   // White
    
    // Borders
    .border          = HAL_RGB(178, 223, 229),   // Light teal border
    .borderFocus     = HAL_RGB(0, 188, 212),     // Teal
    .borderError     = HAL_RGB(244, 67, 54),     // Red
    
    // Status
    .success         = HAL_RGB(76, 175, 80),     // Green
    .warning         = HAL_RGB(255, 193, 7),     // Amber
    .error           = HAL_RGB(244, 67, 54),     // Red
    .info            = HAL_RGB(0, 188, 212),     // Teal
    
    // Shadows
    .shadowAlpha1    = 10,
    .shadowAlpha2    = 18,
    .shadowAlpha3    = 26,
    
    // Typography
    .fontFamily      = "Segoe UI",
    .fontSizeSmall   = 12,
    .fontSizeNormal  = 14,
    .fontSizeLarge   = 16,
    .fontSizeTitle   = 20,
    
    // Spacing
    .spacingXS       = 4,
    .spacingS        = 8,
    .spacingM        = 16,
    .spacingL        = 24,
    .spacingXL       = 32,
    
    // Border radius
    .radiusSmall     = 6,
    .radiusNormal    = 10,
    .radiusLarge     = 14,
    .radiusRound     = 9999,
    
    // Animation
    .animationFast   = 100,
    .animationNormal = 180,
    .animationSlow   = 280
};
