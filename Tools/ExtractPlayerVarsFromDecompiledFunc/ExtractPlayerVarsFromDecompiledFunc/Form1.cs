using System.Text.RegularExpressions;

namespace ExtractPlayerVarsFromDecompiledFunc
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void extractBtn_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(inputText.Text))
                return;

            string decompiledFunc = inputText.Text;
            StringReader reader = new(decompiledFunc);

            if (reader == null)
                return;

            string? line;
            string finalOutput = "";

            while (true)
            {
                line = reader.ReadLine();

                if (line == null)
                {
                    finalOutput = finalOutput.Remove(finalOutput.Length - 1, 1);
                    break;
                }

                if (line.Contains("PlayerFloat", StringComparison.CurrentCultureIgnoreCase))
                {
                    foreach (Match match in Regex.Matches(line, "\"([^\"]*)\""))
                        finalOutput += match.ToString().Trim('"') + ":float,";
                }
                else if (line.Contains("PlayerBool", StringComparison.CurrentCultureIgnoreCase))
                {
                    foreach (Match match in Regex.Matches(line, "\"([^\"]*)\""))
                        finalOutput += match.ToString().Trim('"') + ":bool,";
                }
            }

            outputText.Text = finalOutput;
        }
    }
}
