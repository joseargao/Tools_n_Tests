#pragma once

#include <algorithm>

namespace Bright {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections;
    using namespace System::Windows::Forms;
    using namespace System::Data;
    using namespace System::Drawing;

// Confirm:
// Use fmin and fmax in math.h if supported
float floatMin(float a, float b)
{
    return a < b ? a : b;
}
float floatMax(float a, float b)
{
    return a > b ? a : b;
}

typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} Rgb;

// NOTE: The Hue value needs to be multiplied by 60 to convert it to degrees
typedef struct
{
    float hue;
    float saturation;
    float luminosity;
} Hsl;

typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t brightness;
} RgbSettings;

RgbSettings rgbSettingsStack[10];
static uint8_t rgbSettingsTop = 0;

RgbSettings led0_pop()
{
    if (rgbSettingsTop != 0)
    {
        rgbSettingsTop--;
    }
    RgbSettings settings = rgbSettingsStack[rgbSettingsTop];    return settings;
}

bool led0_push(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    if (rgbSettingsTop > (9))
    {
        return false;
    }
    rgbSettingsStack[rgbSettingsTop].red = red;
    rgbSettingsStack[rgbSettingsTop].green = green;
    rgbSettingsStack[rgbSettingsTop].blue = blue;
    rgbSettingsStack[rgbSettingsTop].brightness = brightness;
    rgbSettingsTop++;
    return true;
}

// Reference:
// http://www.niwa.nu/2013/05/math-behind-colorspace-conversions-rgb-hsl/
Hsl Rgb2Hsl(Rgb sourceColor)
{
    // Normalize RGB values to the range 0-1
    float R = (sourceColor.red / 255.0f);
    float G = (sourceColor.green / 255.0f);
    float B = (sourceColor.blue / 255.0f);

    // Find the minimum and maximum values of R, G and B
    float Min = floatMin(floatMin(R, G), B);
    float Max = floatMax(floatMax(R, G), B);
    float Delta = Max - Min;

    // Calculate the Luminance value by adding the max and min values and divide by 2
    float H = 0;
    float S = 0; // If the min and max value are the same, there is no saturation
    float L = (float)((Max + Min) / 2.0f);

    if (Delta != 0)
    {
        if (L < 0.5f)
        {
            // If Luminance is smaller then 0.5, then Saturation = (max - min) / (max + min)
            S = (float)(Delta / (Max + Min));
        }
        else
        {
            // If Luminance is bigger then 0.5. then Saturation = ( max-min)/(2.0-max-min)
            S = (float)(Delta / (2.0f - Max - Min));
        }

        if (R == Max)
        {
            // If Red is max, then Hue = (G-B)/(max-min)
            H = (G - B) / Delta;
        }
        else if (G == Max)
        {
            // If Green is max, then Hue = 2.0 + (B-R)/(max-min)
            H = 2.0f + (B - R) / Delta;
        }
        else if (B == Max)
        {
            // If Blue is max, then Hue = 4.0 + (R-G)/(max-min)
            H = 4.0f + (R - G) / Delta;
        }
    }

    Hsl destColor;
    destColor.hue = H;
    destColor.saturation = S;
    destColor.luminosity = L;

    return destColor;
}

double GetColor(double temporary_X, double temporary_1, double temporary_2)
{
    // All values need to be between 0 and 1. In our case all the values are between 0 and 1
    // If you get a negative value you need to add 1 to it.
    // If you get a value above 1 you need to subtract 1 from it.
    if (temporary_X < 0) temporary_X += 1.0l;
    if (temporary_X > 1) temporary_X -= 1.0l;

    // If 6 x temporary_X is smaller then 1,
    // Return temporary_2 + (temporary_1 – temporary_2) x 6 x temporary_X
    if (6.0l * temporary_X < 1.0l) return temporary_2 + (temporary_1 - temporary_2) * 6.0l * temporary_X;

    // If 2 x temporary_X is smaller then 1, Return temporary_1
    if (2.0l * temporary_X < 1.0l) return temporary_1;

    // If 3 x temporary_X is smaller then 2,
    // Return temporary_2 + (temporary_1 – temporary_2) x (0.666 – temporary_X) x 6
    if (3.0l * temporary_X < 2.0l) return temporary_2 + (temporary_1 - temporary_2) * (2.0l / 3.0l - temporary_X) * 6.0l;

    // Return temporary_2
    return temporary_2;
}

Rgb Hsl2Rgb(Hsl sourceColor)
{
    unsigned char R, G, B;
    if (sourceColor.saturation == 0)
    {
        // If there is no Saturation it’s a shade of grey
        R = (unsigned char)round(sourceColor.luminosity * 255.0f);
        G = (unsigned char)round(sourceColor.luminosity * 255.0f);
        B = (unsigned char)round(sourceColor.luminosity * 255.0f);
    }
    else
    {
        double temporary_1, temporary_2;

        if (sourceColor.luminosity < 0.5f)
        {
            // If Luminance is smaller then 0.5 (50%) then temporary_1 = Luminance x (1.0+Saturation)
            temporary_1 = sourceColor.luminosity * (1.0f + sourceColor.saturation);
        }
        else
        {
            // If Luminance is equal or larger then 0.5 (50%) then temporary_1 = Luminance + Saturation – Luminance x Saturation
            temporary_1 = (sourceColor.luminosity + sourceColor.saturation) - (sourceColor.luminosity * sourceColor.saturation);
        }
        // temporary_2 = 2 x Luminance – temporary_1
        temporary_2 = 2.0f * sourceColor.luminosity - temporary_1;

        // NOTE: The Hue value needs to be divided by 6 to normalize it (60/360)
        double Hue = sourceColor.hue / 6.0f;

        double temporary_R, temporary_G, temporary_B;
        // temporary_R = Hue + 0.333 = 0.536 + 0.333
        temporary_R = Hue + (1.0f / 3.0f);
        // temporary_G = Hue
        temporary_G = Hue;
        // temporary_B = Hue – 0.333
        temporary_B = Hue - (1.0f / 3.0f);

        // perform final color calculations
        temporary_R = GetColor(temporary_R, temporary_1, temporary_2);
        temporary_G = GetColor(temporary_G, temporary_1, temporary_2);
        temporary_B = GetColor(temporary_B, temporary_1, temporary_2);
        R = (unsigned char)round(temporary_R * 255.0f);
        G = (unsigned char)round(temporary_G * 255.0f);
        B = (unsigned char)round(temporary_B * 255.0f);
    }

    Rgb destColor;
    destColor.red = R;
    destColor.green = G;
    destColor.blue = B;

    return destColor;
}

    /// <summary>
    /// Summary for Main
    /// </summary>
    public ref class Main : public System::Windows::Forms::Form
    {
    public:
        Main(void)
        {
            InitializeComponent();
            //
            //TODO: Add the constructor code here
            //
        }

    protected:
        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        ~Main()
        {
            if (components)
            {
                delete components;
            }
        }
    private: System::Windows::Forms::Button^  button_calcHsl;
    protected:

    protected:

    private: System::Windows::Forms::TextBox^  Red;
    private: System::Windows::Forms::TextBox^  Green;
    private: System::Windows::Forms::TextBox^  Blue;
    private: System::Windows::Forms::TextBox^  Bright;

    private: System::Windows::Forms::Panel^  ColorPanel;







    private: System::Windows::Forms::TextBox^  Hue;
    private: System::Windows::Forms::TextBox^  Saturation;
    private: System::Windows::Forms::TextBox^  Luminosity;
    private: System::Windows::Forms::Button^  button_dispRgb;
    private: System::Windows::Forms::Button^  button_calcRgb;
    private: System::Windows::Forms::Button^  button_calcBright;
    private: System::Windows::Forms::Label^  label1;
    private: System::Windows::Forms::Label^  label2;
    private: System::Windows::Forms::Button^  button_push;
    private: System::Windows::Forms::Button^  button_pop;





    protected:

    private:
        /// <summary>
        /// Required designer variable.
        /// </summary>
        System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        void InitializeComponent(void)
        {
            this->button_calcHsl = (gcnew System::Windows::Forms::Button());
            this->Red = (gcnew System::Windows::Forms::TextBox());
            this->Green = (gcnew System::Windows::Forms::TextBox());
            this->Blue = (gcnew System::Windows::Forms::TextBox());
            this->Bright = (gcnew System::Windows::Forms::TextBox());
            this->ColorPanel = (gcnew System::Windows::Forms::Panel());
            this->Hue = (gcnew System::Windows::Forms::TextBox());
            this->Saturation = (gcnew System::Windows::Forms::TextBox());
            this->Luminosity = (gcnew System::Windows::Forms::TextBox());
            this->button_dispRgb = (gcnew System::Windows::Forms::Button());
            this->button_calcRgb = (gcnew System::Windows::Forms::Button());
            this->button_calcBright = (gcnew System::Windows::Forms::Button());
            this->label1 = (gcnew System::Windows::Forms::Label());
            this->label2 = (gcnew System::Windows::Forms::Label());
            this->button_push = (gcnew System::Windows::Forms::Button());
            this->button_pop = (gcnew System::Windows::Forms::Button());
            this->SuspendLayout();
            // 
            // button_calcHsl
            // 
            this->button_calcHsl->Location = System::Drawing::Point(53, 132);
            this->button_calcHsl->Name = L"button_calcHsl";
            this->button_calcHsl->Size = System::Drawing::Size(165, 23);
            this->button_calcHsl->TabIndex = 0;
            this->button_calcHsl->Text = L"Calculate HSL";
            this->button_calcHsl->UseVisualStyleBackColor = true;
            this->button_calcHsl->Click += gcnew System::EventHandler(this, &Main::button_calcHsl_Click);
            // 
            // Red
            // 
            this->Red->Location = System::Drawing::Point(53, 106);
            this->Red->Name = L"Red";
            this->Red->Size = System::Drawing::Size(51, 20);
            this->Red->TabIndex = 1;
            // 
            // Green
            // 
            this->Green->Location = System::Drawing::Point(110, 106);
            this->Green->Name = L"Green";
            this->Green->Size = System::Drawing::Size(51, 20);
            this->Green->TabIndex = 3;
            // 
            // Blue
            // 
            this->Blue->Location = System::Drawing::Point(167, 106);
            this->Blue->Name = L"Blue";
            this->Blue->Size = System::Drawing::Size(51, 20);
            this->Blue->TabIndex = 4;
            // 
            // Bright
            // 
            this->Bright->Location = System::Drawing::Point(167, 272);
            this->Bright->Name = L"Bright";
            this->Bright->Size = System::Drawing::Size(51, 20);
            this->Bright->TabIndex = 5;
            // 
            // ColorPanel
            // 
            this->ColorPanel->BackColor = System::Drawing::SystemColors::Control;
            this->ColorPanel->Location = System::Drawing::Point(0, 0);
            this->ColorPanel->Name = L"ColorPanel";
            this->ColorPanel->Size = System::Drawing::Size(224, 100);
            this->ColorPanel->TabIndex = 7;
            // 
            // Hue
            // 
            this->Hue->Location = System::Drawing::Point(53, 215);
            this->Hue->Name = L"Hue";
            this->Hue->Size = System::Drawing::Size(51, 20);
            this->Hue->TabIndex = 8;
            // 
            // Saturation
            // 
            this->Saturation->Location = System::Drawing::Point(110, 215);
            this->Saturation->Name = L"Saturation";
            this->Saturation->Size = System::Drawing::Size(51, 20);
            this->Saturation->TabIndex = 9;
            // 
            // Luminosity
            // 
            this->Luminosity->Location = System::Drawing::Point(167, 215);
            this->Luminosity->Name = L"Luminosity";
            this->Luminosity->Size = System::Drawing::Size(51, 20);
            this->Luminosity->TabIndex = 10;
            // 
            // button_dispRgb
            // 
            this->button_dispRgb->Location = System::Drawing::Point(53, 159);
            this->button_dispRgb->Name = L"button_dispRgb";
            this->button_dispRgb->Size = System::Drawing::Size(165, 23);
            this->button_dispRgb->TabIndex = 11;
            this->button_dispRgb->Text = L"Display RGB";
            this->button_dispRgb->UseVisualStyleBackColor = true;
            this->button_dispRgb->Click += gcnew System::EventHandler(this, &Main::button_dispRgb_Click);
            // 
            // button_calcRgb
            // 
            this->button_calcRgb->Location = System::Drawing::Point(53, 241);
            this->button_calcRgb->Name = L"button_calcRgb";
            this->button_calcRgb->Size = System::Drawing::Size(165, 23);
            this->button_calcRgb->TabIndex = 12;
            this->button_calcRgb->Text = L"Calculate RGB";
            this->button_calcRgb->UseVisualStyleBackColor = true;
            this->button_calcRgb->Click += gcnew System::EventHandler(this, &Main::button_calcRgb_Click);
            // 
            // button_calcBright
            // 
            this->button_calcBright->Location = System::Drawing::Point(53, 270);
            this->button_calcBright->Name = L"button_calcBright";
            this->button_calcBright->Size = System::Drawing::Size(108, 23);
            this->button_calcBright->TabIndex = 13;
            this->button_calcBright->Text = L"Apply Brightness";
            this->button_calcBright->UseVisualStyleBackColor = true;
            this->button_calcBright->Click += gcnew System::EventHandler(this, &Main::button_calcBright_Click);
            // 
            // label1
            // 
            this->label1->AutoSize = true;
            this->label1->Location = System::Drawing::Point(7, 109);
            this->label1->Name = L"label1";
            this->label1->Size = System::Drawing::Size(30, 13);
            this->label1->TabIndex = 14;
            this->label1->Text = L"RGB";
            // 
            // label2
            // 
            this->label2->AutoSize = true;
            this->label2->Location = System::Drawing::Point(7, 218);
            this->label2->Name = L"label2";
            this->label2->Size = System::Drawing::Size(28, 13);
            this->label2->TabIndex = 15;
            this->label2->Text = L"HSL";
            // 
            // button_push
            // 
            this->button_push->Location = System::Drawing::Point(53, 188);
            this->button_push->Name = L"button_push";
            this->button_push->Size = System::Drawing::Size(81, 23);
            this->button_push->TabIndex = 16;
            this->button_push->Text = L"Push";
            this->button_push->UseVisualStyleBackColor = true;
            this->button_push->Click += gcnew System::EventHandler(this, &Main::button_push_Click);
            // 
            // button_pop
            // 
            this->button_pop->Location = System::Drawing::Point(137, 188);
            this->button_pop->Name = L"button_pop";
            this->button_pop->Size = System::Drawing::Size(81, 23);
            this->button_pop->TabIndex = 17;
            this->button_pop->Text = L"Pop";
            this->button_pop->UseVisualStyleBackColor = true;
            this->button_pop->Click += gcnew System::EventHandler(this, &Main::button_pop_Click);
            // 
            // Main
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(224, 296);
            this->Controls->Add(this->button_pop);
            this->Controls->Add(this->button_push);
            this->Controls->Add(this->label2);
            this->Controls->Add(this->label1);
            this->Controls->Add(this->button_calcBright);
            this->Controls->Add(this->button_calcRgb);
            this->Controls->Add(this->button_dispRgb);
            this->Controls->Add(this->Luminosity);
            this->Controls->Add(this->Saturation);
            this->Controls->Add(this->Hue);
            this->Controls->Add(this->ColorPanel);
            this->Controls->Add(this->Bright);
            this->Controls->Add(this->Blue);
            this->Controls->Add(this->Green);
            this->Controls->Add(this->Red);
            this->Controls->Add(this->button_calcHsl);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::Fixed3D;
            this->MaximizeBox = false;
            this->MinimizeBox = false;
            this->Name = L"Main";
            this->Text = L"RGB-HSL Tester";
            this->ResumeLayout(false);
            this->PerformLayout();

        }
#pragma endregion

    private: System::Void button_dispRgb_Click(System::Object^  sender, System::EventArgs^  e) {
        int R, G, B;

        if (System::String::IsNullOrEmpty(this->Red->Text) ||
            System::String::IsNullOrEmpty(this->Green->Text) || 
            System::String::IsNullOrEmpty(this->Blue->Text))
        {
            return;
        }

        R = System::Convert::ToInt32(this->Red->Text);
        G = System::Convert::ToInt32(this->Green->Text);
        B = System::Convert::ToInt32(this->Blue->Text);

        this->ColorPanel->BackColor = Color::FromArgb(R, G, B);
    }
    private: System::Void button_calcHsl_Click(System::Object^  sender, System::EventArgs^  e) {
        int R, G, B;

        R = System::Convert::ToInt32(this->Red->Text);
        G = System::Convert::ToInt32(this->Green->Text);
        B = System::Convert::ToInt32(this->Blue->Text);

        Rgb sourceColor;
        sourceColor.red = R;
        sourceColor.green = G;
        sourceColor.blue = B;

        Hsl destColor = Rgb2Hsl(sourceColor);
        this->Hue->Text = destColor.hue.ToString();
        this->Saturation->Text = destColor.saturation.ToString();
        this->Luminosity->Text = destColor.luminosity.ToString();
    }
    private: System::Void button_calcRgb_Click(System::Object^  sender, System::EventArgs^  e) {
        float H, S, L;

        H = System::Convert::ToDouble(this->Hue->Text);
        S = System::Convert::ToDouble(this->Saturation->Text);
        L = System::Convert::ToDouble(this->Luminosity->Text);

        Hsl sourceColor;
        sourceColor.hue = H;
        sourceColor.saturation = S;
        sourceColor.luminosity = L;

        Rgb destColor = Hsl2Rgb(sourceColor);
        this->Red->Text = destColor.red.ToString();
        this->Green->Text = destColor.green.ToString();
        this->Blue->Text = destColor.blue.ToString();
    }
    private: System::Void button_dispHSL_Click(System::Object^  sender, System::EventArgs^  e) {
        float H, S, L, R, G, B;

        H = System::Convert::ToDouble(this->Hue->Text);
        S = System::Convert::ToDouble(this->Saturation->Text);
        L = System::Convert::ToDouble(this->Luminosity->Text);

        Hsl sourceColor;
        sourceColor.hue = H;
        sourceColor.saturation = S;
        sourceColor.luminosity = L;

        Rgb destColor = Hsl2Rgb(sourceColor);
        this->Red->Text = destColor.red.ToString();
        this->Green->Text = destColor.green.ToString();
        this->Blue->Text = destColor.blue.ToString();

        if (System::String::IsNullOrEmpty(this->Red->Text) ||
            System::String::IsNullOrEmpty(this->Green->Text) ||
            System::String::IsNullOrEmpty(this->Blue->Text))
        {
            return;
        }

        R = System::Convert::ToInt32(this->Red->Text);
        G = System::Convert::ToInt32(this->Green->Text);
        B = System::Convert::ToInt32(this->Blue->Text);

        this->ColorPanel->BackColor = Color::FromArgb(R, G, B);
    }
    private: System::Void button_calcBright_Click(System::Object^  sender, System::EventArgs^  e) {
        float L, B;
        B = System::Convert::ToDouble(this->Bright->Text);
        L = System::Convert::ToDouble(this->Luminosity->Text);

        this->Luminosity->Text = (B*L).ToString();
    }

    private: System::Void button_push_Click(System::Object^  sender, System::EventArgs^  e) {
        int R, G, B;

        if (System::String::IsNullOrEmpty(this->Red->Text) ||
            System::String::IsNullOrEmpty(this->Green->Text) ||
            System::String::IsNullOrEmpty(this->Blue->Text))
        {
            return;
        }

        R = System::Convert::ToInt32(this->Red->Text);
        G = System::Convert::ToInt32(this->Green->Text);
        B = System::Convert::ToInt32(this->Blue->Text);

        this->ColorPanel->BackColor = Color::FromArgb(R, G, B);
        led0_push(R, G, B, 255);
    }
    private: System::Void button_pop_Click(System::Object^  sender, System::EventArgs^  e) {
        RgbSettings settings = led0_pop();
        this->ColorPanel->BackColor = Color::FromArgb(settings.red, settings.green, settings.blue);
    }
};
}
