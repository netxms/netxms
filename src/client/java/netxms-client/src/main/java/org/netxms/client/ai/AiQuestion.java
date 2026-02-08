/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2025 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.ai;

import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * AI agent question for user interaction
 */
public class AiQuestion
{
   /**
    * Confirmation type for binary questions
    */
   public enum ConfirmationType
   {
      APPROVE_REJECT,
      YES_NO,
      CONFIRM_CANCEL
   }

   private long id;
   private int chatId;
   private boolean multipleChoice;
   private ConfirmationType confirmationType;
   private String text;
   private String context;
   private List<String> options;
   private int timeoutSeconds;

   /**
    * Create question object from NXCP message.
    *
    * @param msg NXCP message
    */
   public AiQuestion(NXCPMessage msg)
   {
      id = msg.getFieldAsInt64(NXCPCodes.VID_AI_QUESTION_ID);
      chatId = msg.getFieldAsInt32(NXCPCodes.VID_CHAT_ID);
      multipleChoice = msg.getFieldAsInt32(NXCPCodes.VID_AI_QUESTION_TYPE) == 1;
      int confType = msg.getFieldAsInt32(NXCPCodes.VID_AI_CONFIRMATION_TYPE);
      confirmationType = (confType >= 0 && confType < ConfirmationType.values().length)
         ? ConfirmationType.values()[confType]
         : ConfirmationType.APPROVE_REJECT;
      text = msg.getFieldAsString(NXCPCodes.VID_AI_QUESTION_TEXT);
      context = msg.getFieldAsString(NXCPCodes.VID_AI_QUESTION_CONTEXT);
      options = msg.getFieldAsStringList(NXCPCodes.VID_AI_QUESTION_OPTIONS);
      timeoutSeconds = msg.getFieldAsInt32(NXCPCodes.VID_AI_QUESTION_TIMEOUT);
   }

   /**
    * Get question ID.
    *
    * @return question ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Get chat ID this question belongs to.
    *
    * @return chat ID
    */
   public int getChatId()
   {
      return chatId;
   }

   /**
    * Check if this is a multiple choice question.
    *
    * @return true if multiple choice, false if binary confirmation
    */
   public boolean isMultipleChoice()
   {
      return multipleChoice;
   }

   /**
    * Get confirmation type for binary questions.
    *
    * @return confirmation type
    */
   public ConfirmationType getConfirmationType()
   {
      return confirmationType;
   }

   /**
    * Get positive button label based on confirmation type.
    *
    * @return positive button label
    */
   public String getPositiveLabel()
   {
      switch(confirmationType)
      {
         case APPROVE_REJECT:
            return "Approve";
         case YES_NO:
            return "Yes";
         case CONFIRM_CANCEL:
            return "Confirm";
         default:
            return "Yes";
      }
   }

   /**
    * Get negative button label based on confirmation type.
    *
    * @return negative button label
    */
   public String getNegativeLabel()
   {
      switch(confirmationType)
      {
         case APPROVE_REJECT:
            return "Reject";
         case YES_NO:
            return "No";
         case CONFIRM_CANCEL:
            return "Cancel";
         default:
            return "No";
      }
   }

   /**
    * Get question text.
    *
    * @return question text
    */
   public String getText()
   {
      return text;
   }

   /**
    * Get additional context for the question (e.g., command to be approved).
    *
    * @return context string, may be empty
    */
   public String getContext()
   {
      return context;
   }

   /**
    * Get options for multiple choice questions.
    *
    * @return list of options, empty for binary questions
    */
   public List<String> getOptions()
   {
      return options;
   }

   /**
    * Get remaining timeout in seconds.
    *
    * @return timeout in seconds
    */
   public int getTimeoutSeconds()
   {
      return timeoutSeconds;
   }
}
