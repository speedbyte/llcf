import Tkinter


class App:

     def __init__(self, parent):

         frame = Tkinter.Frame(parent)
         frame.pack()

         self.state1 = Tkinter.Button(frame, text ="MDI 01", bg="red", fg="black", command=parent.quit)
         self.state2 = Tkinter.Button(frame, text ="MDI 02", bg="red", fg="black", command=parent.quit)
         self.state3 = Tkinter.Button(frame, text ="MDI 03", bg="red", fg="black", command=parent.quit)
         self.state4 = Tkinter.Button(frame, text ="MDI 04", bg="red", fg="black", command=parent.quit)
         self.state5 = Tkinter.Button(frame, text ="MDI 05", bg="red", fg="black", command=parent.quit)
         self.state6 = Tkinter.Button(frame, text ="MDI 06", bg="red", fg="black", command=parent.quit)
         self.state7 = Tkinter.Button(frame, text ="MDI 07", bg="red", fg="black", command=parent.quit)
         self.state8 = Tkinter.Button(frame, text ="MDI 08", bg="red", fg="black", command=parent.quit)
         self.state9 = Tkinter.Button(frame, text ="MDI 09", bg="red", fg="black", command=parent.quit)
         self.state10 = Tkinter.Button(frame, text="MDI 10", bg="red", fg="black", command=parent.quit)


         self.state1.grid(row=0, column=0, sticky=Tkinter.W)
         self.state2.grid(row=1, column=0, sticky=Tkinter.W)
         self.state3.grid(row=2, column=0, sticky=Tkinter.W)
         self.state4.grid(row=3, column=0, sticky=Tkinter.W)
         self.state5.grid(row=4, column=0, sticky=Tkinter.W)

         self.state6.grid(row=0, column=1, sticky=Tkinter.W)
         self.state7.grid(row=1, column=1, sticky=Tkinter.W)
         self.state8.grid(row=2, column=1, sticky=Tkinter.W)
         self.state9.grid(row=3, column=1, sticky=Tkinter.W)
         self.state10.grid(row=4, column=1, sticky=Tkinter.W)

         self.state1.bg = "green"

root = Tkinter.Tk()

app = App(root)

root.mainloop()
