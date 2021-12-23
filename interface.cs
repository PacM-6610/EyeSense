using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EyeSense_GUI
{
    public partial class Eyesense : Form
    {
        public delegate void d1(string indata);
        public Eyesense()
        {
            InitializeComponent();
            serialPort1.Open();
        }

        private void STOP_Click(object sender, EventArgs e)
        {
            serialPort1.Write("B");
        }

        private void serialPort1_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            string indata = serialPort1.ReadLine();
            d1 writeit = new d1(WriteToForm);
            Invoke(writeit, indata);
        }

        public void WriteToForm(string indata)
        {
            //this function handles data sent from the arduino
            char firstChar;
            char secondChar = 'z';
            int numdata;
            string value;
            int intValue;
            numdata = indata.Length;
            firstChar = indata[0];
            if (numdata >= 2) { secondChar = indata[1]; }
            switch (firstChar)
            {
                case 'a'://ultrasonic sensor 1
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US1.Text =Convert.ToString(value);
                    break;
                case 'b':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US2.Text = Convert.ToString(value);
                    break;
                case 'c':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US3.Text = Convert.ToString(value);
                    break;
                case 'd':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US4.Text = Convert.ToString(value);
                    break;
                case 'e':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US5.Text = Convert.ToString(value);
                    break;
                case 'f':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US6.Text = Convert.ToString(value);
                    break;
                case 'g':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    US7.Text = Convert.ToString(value);
                    break;
                case 'h':
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    Load_Cell_Force.Text = Convert.ToString(value);
                    break;
                case 'i'://motorSpeed
                    if (secondChar == 'n')//if negative
                    {
                        value = new String(indata.Where(Char.IsDigit).ToArray());
                        motorSpeed.Text = ('-' + Convert.ToString(value));
                        progressBarUntightening.Value = Convert.ToInt32(value);
                        progressBarTightening.Value = 0;
                    }
                    else
                    {
                        value = new String(indata.Where(Char.IsDigit).ToArray());
                        motorSpeed.Text = Convert.ToString(value);
                        progressBarUntightening.Value = 0;
                        progressBarTightening.Value = Convert.ToInt32(value);
                    }
                    break;
                case 'j'://vibrator
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    intValue = Convert.ToInt32(value);
                    Vib1.Visible = false;
                    Vib2.Visible = false;
                    Vib3.Visible = false;
                    Vib4.Visible = false;
                    Vib5.Visible = false;
                    Vib6.Visible = false;

                    if (intValue == 100){ break; }
                    else if(intValue == 0)
                    {
                        Vib1.Visible = true;
                        Vib6.Visible = true;
                    }
                    else if (intValue == 1) { Vib1.Visible = true; }
                    else if (intValue == 2) { Vib2.Visible = true; }
                    else if (intValue == 3) { Vib3.Visible = true; }
                    else if (intValue == 4) { Vib4.Visible = true; }
                    else if (intValue == 5) { Vib5.Visible = true; }
                    else if (intValue == 6) { Vib6.Visible = true; }
                    break;
                case 'k'://Is STOP ?
                    value = new String(indata.Where(Char.IsDigit).ToArray());
                    intValue = Convert.ToInt32(value);
                    if (intValue == 1)
                    {
                        IsSTOP.Text = "Stopped";
                    }
                    else
                    {
                        IsSTOP.Text = "Running";
                    }
                    break;
            }
        }
    }
}
