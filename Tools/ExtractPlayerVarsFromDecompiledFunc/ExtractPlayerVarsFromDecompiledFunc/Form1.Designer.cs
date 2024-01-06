namespace ExtractPlayerVarsFromDecompiledFunc
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            inputText = new RichTextBox();
            inputTextLabel = new Label();
            outputText = new RichTextBox();
            extractBtn = new Button();
            SuspendLayout();
            // 
            // inputText
            // 
            inputText.AcceptsTab = true;
            inputText.DetectUrls = false;
            inputText.EnableAutoDragDrop = true;
            inputText.Location = new Point(12, 35);
            inputText.Name = "inputText";
            inputText.ScrollBars = RichTextBoxScrollBars.ForcedVertical;
            inputText.Size = new Size(716, 226);
            inputText.TabIndex = 0;
            inputText.Text = "";
            // 
            // inputTextLabel
            // 
            inputTextLabel.AutoSize = true;
            inputTextLabel.Location = new Point(12, 9);
            inputTextLabel.Name = "inputTextLabel";
            inputTextLabel.Size = new Size(121, 15);
            inputTextLabel.TabIndex = 1;
            inputTextLabel.Text = "Decompiled Function";
            // 
            // outputText
            // 
            outputText.AcceptsTab = true;
            outputText.DetectUrls = false;
            outputText.EnableAutoDragDrop = true;
            outputText.Location = new Point(12, 301);
            outputText.Name = "outputText";
            outputText.ReadOnly = true;
            outputText.ScrollBars = RichTextBoxScrollBars.ForcedVertical;
            outputText.Size = new Size(716, 226);
            outputText.TabIndex = 1;
            outputText.Text = "";
            // 
            // extractBtn
            // 
            extractBtn.Location = new Point(299, 267);
            extractBtn.Name = "extractBtn";
            extractBtn.Size = new Size(125, 28);
            extractBtn.TabIndex = 2;
            extractBtn.Text = "Extract Player Vars";
            extractBtn.UseVisualStyleBackColor = true;
            extractBtn.Click += extractBtn_Click;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(740, 539);
            Controls.Add(extractBtn);
            Controls.Add(outputText);
            Controls.Add(inputTextLabel);
            Controls.Add(inputText);
            MaximizeBox = false;
            Name = "Form1";
            ShowIcon = false;
            Text = "Extract player variables from decompiled function";
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private RichTextBox inputText;
        private Label inputTextLabel;
        private RichTextBox outputText;
        private Button extractBtn;
    }
}
