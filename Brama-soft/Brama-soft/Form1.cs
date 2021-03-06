﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;

namespace Brama_soft
{
    public partial class Form1 : Form
    {

        SerialPort port;

        public Form1()
        {
            InitializeComponent();
            RefreshPorts();
        }

        private void Connected()
        {
            button5.Enabled = true;
            button7.Enabled = false;
            button3.Enabled = true;
            button4.Enabled = true;
            button6.Enabled = false;
            comboBox1.Enabled = false;
            button8.Enabled = true;
        }

        private void Disconnected()
        {
            button5.Enabled = false;
            button7.Enabled = true;
            button3.Enabled = false;
            button4.Enabled = false;
            button6.Enabled = true;
            comboBox1.Enabled = true;
            button8.Enabled = false;
        }

        private void RefreshPorts()
        {
            comboBox1.Items.Clear();
            comboBox1.Items.AddRange(SerialPort.GetPortNames());
        }

        void port_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            while (port.BytesToRead > 0 && port.IsOpen)
            {
                Log(((char)port.ReadChar()).ToString());
            }

        }

        private void button7_Click(object sender, EventArgs e)
        {
            if (port != null && port.IsOpen)
                port.Close();

            port = new SerialPort(comboBox1.SelectedItem.ToString());
            port.BaudRate = 9600;
            port.Parity = Parity.None;
            port.DataBits = 8;
            port.StopBits = StopBits.One;
            port.Handshake = Handshake.None;

            port.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);

            port.Open();

            label4.Text = port.PortName;

            Connected();
        }

        private void button6_Click(object sender, EventArgs e)
        {
            RefreshPorts();
        }

        private void button5_Click(object sender, EventArgs e)
        {
            if (port != null && port.IsOpen)
                port.Close();

            label4.Text = "--";

            Disconnected();
        }


        private void Log(string text)
        {
            if (this.InvokeRequired)
                this.Invoke(new MethodInvoker(delegate
                {
                    textBox2.Text += text;
                }));
            else
            {
                textBox2.Text += text;
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (listBox1.Items.Count < 1)
            {
                MessageBox.Show("Brak numerów!", "Błąd", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            
            port.Write("*");

            for (int i = 0; i < listBox1.Items.Count; i++)
            {
                port.Write(listBox1.Items[i].ToString());

                if(i == listBox1.Items.Count - 1)
                    port.Write(((char)0x1A).ToString());
                else
                    port.Write(((char)0x0D).ToString());
            }

        }

        private void button4_Click(object sender, EventArgs e)
        {
            port.Write(((char)0x1B).ToString());
        }

        private void button1_Click(object sender, EventArgs e)
        {
            long liczba = 0;
            bool b = long.TryParse(textBox1.Text, out liczba);

            if (!b || liczba < 100000000 || liczba > 999999999)
            {
                MessageBox.Show("To nie jest numer telefonu!", "Błąd", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }


            listBox1.Items.Add(textBox1.Text);
            textBox1.Text = "";
        }

        private void button2_Click(object sender, EventArgs e)
        {
            listBox1.Items.Clear();
        }

        private void textBox1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
                button1_Click(null, null);
        }

        private void button9_Click(object sender, EventArgs e)
        {
            textBox2.Clear();
        }

        private void button8_Click(object sender, EventArgs e)
        {
            port.Write("d");
        }
    }
}
